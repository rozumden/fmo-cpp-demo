#include "algorithm-taxonomy.hpp"
#include "../image-util.hpp"
#include <fmo/region.hpp>
#include <fmo/assert.hpp>
#include <numeric>
#include <opencv/cv.hpp>

namespace fmo {
    void TaxonomyV1::findComponents() {
        // calculate final distance tranform
        distance_transform(mProcessingLevel.binDiff, mProcessingLevel.distTran);

        // local maxima calculation
        local_maxima(mProcessingLevel.distTran, mProcessingLevel.localMaxima);
        mProcessingLevel.objectsNow = cv::connectedComponentsWithStats(
                          mProcessingLevel.binDiff.wrap(),
                          mProcessingLevel.labels.wrap(),
                          mProcessingLevel.stats,
                          mProcessingLevel.centroids, 8);

    }

    void TaxonomyV1::Component::calcIoU() {
        auto thickness = roundf(2.f * this->radius);
        Image temp;
        temp.resize(Format::GRAY, this->size);

        cv::Mat buf = temp.wrap();
        buf.setTo(uint8_t(0));
        
        this->curve->shift = {this->start.x,this->start.y};
        this->curve->draw(buf, 1, thickness);
        this->curve->shift = {0,0};

        int inters = 0;

        for(auto &pix : this->pixels) {
            if (buf.at<int8_t>(pix-start) > 0)
                inters++;
        }

        int unio = this->pixels.size() + cv::countNonZero(buf) - inters;
        this->iou = (float)inters / (float)unio;
    }

    void TaxonomyV1::processComponents() {
        bool realTime = true;

    	mObjects.clear();
        mPrevComponents.swap(mComponents);
    	mComponents.clear();
    	float w = mProcessingLevel.newDims.width, h = mProcessingLevel.newDims.height;
        float area = w*h;

        int32_t diffArea = 0;
    	for (int i = 0; i < int(mProcessingLevel.objectsNow-1); ++i) {
    		mComponents.emplace_back(Component());
            auto& comp = mComponents[i];
            comp.id = i+1;
            comp.area = mProcessingLevel.stats.at<int>(comp.id,cv::CC_STAT_AREA);
            if(realTime) diffArea += comp.area;
    		if(comp.area < 10)
                comp.status = Component::TOO_SMALL;
    		else if(realTime && comp.area > 0.1*area)
                comp.status = Component::TOO_LARGE;
     	}

        // probably big illumintation change
        if (realTime && diffArea > 0.3*area) {
            for(auto& comp : mComponents)
                comp.status = Component::TOO_LARGE;
            return;
        }

        cv::Mat labels = mProcessingLevel.labels.wrap();
        cv::Mat dt = mProcessingLevel.distTran.wrap();
        cv::Mat lm = mProcessingLevel.localMaxima.wrap();
    	int32_t* p;
    	float* p2;
    	uint8_t* p3;
    	for(int row = 0; row < h; ++row) {
	        p = labels.ptr<int32_t>(row);
	        p2 = dt.ptr<float>(row);
	        p3 = lm.ptr<uint8_t>(row);
	        for (int column = 0; column < w; ++column) {
                if(p2[column] < 1.5) continue;
	        	int ind = p[column];	
	    		if(ind > 0 && mComponents[ind-1].status == Component::NOT_PROCESSED) {
                    mComponents[ind-1].pixels.emplace_back(column,row);
	    			if(p3[column] == 255) {
	    				mComponents[ind-1].dist.emplace_back(p2[column]);
	    				mComponents[ind-1].traj.emplace_back(column,row);
	    			}
	    		}
	        }
	    }

        std::vector<float> dists;
    	for(auto& comp : mComponents) {
			if(comp.dist.size() == 0) comp.status = Component::TOO_SMALL;

			if(comp.status != Component::NOT_PROCESSED) continue; // just to make sure

			// get radius
            dists = comp.dist;
	    	int n = round(2*dists.size()/3);
	    	std::nth_element(dists.begin(), dists.begin()+n, dists.end());
	    	comp.radius = dists[n] + 0.5;
            if(comp.radius < 4) {
                comp.status = Component::TOO_SMALL;
                continue;
            }

			// get all stats about this component
    		comp.start.x = mProcessingLevel.stats.at<int>(comp.id,cv::CC_STAT_LEFT);
    		comp.start.y = mProcessingLevel.stats.at<int>(comp.id,cv::CC_STAT_TOP);
			comp.size.width = mProcessingLevel.stats.at<int>(comp.id,cv::CC_STAT_WIDTH);
    		comp.size.height = mProcessingLevel.stats.at<int>(comp.id,cv::CC_STAT_HEIGHT);
		 	comp.center[0] = mProcessingLevel.centroids.at<double>(comp.id,0);
		 	comp.center[1] = mProcessingLevel.centroids.at<double>(comp.id,1);

			// get local maxima pixels on trajectory
    		for (int i = 0; i < int(comp.dist.size()); ++i) {
                float th = 0.7*comp.radius-1;
                if(comp.dist[i] > th) {
    				comp.trajFinal.push_back(comp.traj[i]);
    			} else if(!realTime) {
                    comp.otherLM.push_back(comp.traj[i]);
				}
    		}

            int np = comp.trajFinal.size();
			// minimal number of points needed for fitting
    		if(np <= 3) {
    			comp.status = Component::TOO_SMALL;
    			continue;
    		}

            /// fit curve
            float score = fmo::fitcurve(comp.trajFinal, comp.radius, comp.curve, comp.circle, comp.line);

			comp.len = comp.curve->length;
			if(score == 0) { // score threshold
                comp.status = Component::NOT_STROKE;
                continue;
            }

            /// check FMO model (IoU with CC)
            comp.calcIoU();
			if(comp.iou < 0.6) {
				comp.status = Component::NOT_STROKE;
				continue;
			}
            comp.maxDist = comp.curve->maxDist(comp.trajFinal);

    		///////////////////////////////////////////////////////////

    		float count = 0;
            cv::Mat mat = mProcessingLevel.binDiffPrev.wrap();
            int edgePixels = 0;
    		for(auto& pix : comp.pixels) {
                if (pix.x == 0 || pix.y == 0 || pix.x == (w-1) || pix.y == (h-1)) edgePixels++;
    			if (mat.at<uchar>(pix.y,pix.x)) count ++;
            }

    		if(count/comp.area > 0.4 || edgePixels > 2*comp.radius){
    			comp.status = Component::LATERAL;
    			continue;
    		}
            

            comp.status = Component::FMO_NOT_CONFIRMED;

            ///////////////// Check with previous detections  /////////////////////////////////////////
            float maxScore = 0;
            float maxAllowedExp = 3;
            int ind = -1;
            for (unsigned int jj = 0; jj < mPrevComponents.size(); ++jj) {
                auto& compOld = mPrevComponents[jj];
                if(compOld.status != Component::FMO && compOld.status != Component::FMO_NOT_CONFIRMED)
                    continue;
                float d = cv::norm(comp.center - compOld.center);
                if(d > maxAllowedExp*comp.len) continue;
                compOld.trajFinal.insert(compOld.trajFinal.end(), comp.trajFinal.begin(), comp.trajFinal.end());

                float s = fmo::fitcurve(compOld.trajFinal, comp.radius, compOld.curve,
                                        compOld.circle, compOld.line, compOld.center, comp.center);
                if(compOld.curve->length > maxAllowedExp*comp.len) s = 0;
                if(compOld.curve->length < comp.len) s = 0;
                if(s > maxScore) {
                    ind = jj;
                    maxScore = s;
                }
            }

            if (maxScore == 0) continue;
            comp.curveSmooth = mPrevComponents[ind].curve->clone();

            float maxDist = mPrevComponents[ind].curve->maxDist(mPrevComponents[ind].trajFinal);
            if(maxDist > comp.radius) {
                continue;
            }

            comp.len = comp.curveSmooth->length;
            ///////////////////////////////////////////////////////////////
            comp.status = Component::FMO;

            Object o;
    		o.center = Pos{(int)(comp.center[0]/mProcessingLevel.scale),
    					   (int)(comp.center[1]/mProcessingLevel.scale)};
    		o.direction = NormVector{comp.line.normal.x,comp.line.normal.y};
    		o.length = comp.len/mProcessingLevel.scale; 
    		o.radius = comp.radius/mProcessingLevel.scale;
            o.curve = comp.curve;
            o.curveSmooth = comp.curveSmooth;
            o.velocity = comp.len / (o.radius+1.5); // in radii per exposure

            mObjects.push_back(o);
    	}
    }
}
