#include "../include-opencv.hpp"
#include "algorithm-taxonomy.hpp"
#include "../image-util.hpp"
#include <fmo/processing.hpp>
#include <iostream>
#include <fmo/assert.hpp>

namespace fmo {
    namespace {
        const cv::Scalar colorRed{0x00, 0x00, 0xFF};
        const cv::Vec3b colorRedVec{0x00, 0x00, 0xFF};
        const cv::Scalar colorRedDark{0xC0, 0xC0, 0xFF};
        const cv::Scalar colorBlue{0xFF, 0x00, 0x00};
        const cv::Vec3b colorBlueVec{0xFF, 0x00, 0x00};
        const cv::Scalar colorGreen{0x00, 0xFF, 0x00}; 
        const cv::Scalar colorYellow{0x00, 0xFF, 0xFF}; 
        const cv::Scalar colorViolet{0xFF, 0x00, 0xFF}; 
        const cv::Scalar colorWhite{0xFF, 0xFF, 0xFF}; 
        const cv::Scalar colorObjects[3] = {
            {0x00, 0xC0, 0x00}, {0x00, 0x80, 0x00}, {0x00, 0x40, 0x00},
        };
        const cv::Scalar colorSelected{0x00, 0xC0, 0xC0};
    }

    const Image& TaxonomyV1::getDebugImage() {
        return getDebugImage(3, true, true, 3);
    }

    const Image& TaxonomyV1::getDebugImage(int level, bool showIm, bool showLM, int add) 
    {
        cv::Scalar objColor = cv::Scalar(0,0,0xFF);
        cv::Mat cvVis = mCache.visualized.wrap();
        cv::Mat cvVisFull = mCache.visualizedFull.wrap();
        cv::Mat cvDTBGR = mCache.distTranBGR.wrap();
        cv::Mat cvDT = mProcessingLevel.distTran.wrap();
        if (showIm) {
            fmo::copy(mProcessingLevel.inputs[0], mCache.visualized);
        } else {
            cvVis.setTo(0);
        }

        if (add == 1) {
            std::vector<cv::Mat> channels;
            channels.push_back(mProcessingLevel.binDiff.wrap());
            channels.push_back(mProcessingLevel.binDiff.wrap());
            channels.push_back(mProcessingLevel.binDiff.wrap());
            cv::merge(channels, cvVis);
            if (showIm) {
                cvVis = cvVis / 255;
                cvVis = cvVis.mul(mProcessingLevel.inputs[0].wrap());
            }
        } else if (add == 2) {
            fmo::copy(mProcessingLevel.diff, mCache.visualized);
        } else if (add == 3) {
            double min, max;
            cv::minMaxLoc(cvDT, &min, &max);
            mCache.distTranReverse.wrap() = 255*((cvDT / max));
            mCache.distTranReverse.wrap().convertTo(mCache.distTranGray.wrap(), fmo::getCvType(Format::GRAY));
            
            fmo::copy(mCache.distTranGray, mCache.distTranBGR, Format::BGR);
            cv::addWeighted(cvDTBGR, 0.7, cvVis, 0.3, 0, cvVis);
        }

        if (false) {
            std::vector<cv::Mat> channels;
            cv::Mat mask = 1 - (mProcessingLevel.localMaxima.wrap()/255);
            channels.push_back(mask);
            channels.push_back(mask);
            channels.push_back(mCache.ones.wrap()+mProcessingLevel.localMaxima.wrap());
            cv::merge(channels, cvDTBGR);
            cvVis = cvVis.mul(cvDTBGR);
        }

        cv::resize(cvVis, cvVisFull, cvVisFull.size(), 0, 0, cv::INTER_NEAREST);
        int objInd = 0;
        char str[6];

        for (auto& comp : mComponents) {
            const cv::Scalar* color = &colorRed;
            if (comp.status == Component::TOO_SMALL || 
                comp.status == Component::TOO_LARGE) 
            { 
                continue;
            }
            bool cont = false;

            if (comp.status == Component::NOT_STROKE) { 
                if (level < 4) cont = true;
                color = &colorViolet; 
            }
            if (comp.status == Component::LATERAL) { 
                if (level < 3) cont = true;
                color = &colorBlue; 
            }
            if (comp.status == Component::SHADOW) { 
                if (level < 2) cont = true;
                color = &colorYellow; 
            }
            if (comp.status == Component::FMO_NOT_CONFIRMED) {
                if (level < 2) cont = true;
                color = &colorRed;
            }
            if (comp.status == Component::FMO) { 
                if (level < 1) cont = true;
                color = &colorGreen; 
            }

            comp.curve->scale = mProcessingLevel.scale;
            if(!cont) {
                comp.draw(cvVisFull);
                if(comp.curveSmooth != nullptr) {
                    comp.curveSmooth->scale = mProcessingLevel.scale;
                    comp.curveSmooth->drawSmooth(cvVisFull, objColor, 1);
                }
            }

            if (showLM) {
                for (auto &pnt : comp.trajFinal)
                    cvVisFull.at<cv::Vec3b>(pnt / mProcessingLevel.scale) = colorBlueVec;

                for (auto &pnt : comp.otherLM)
                    cvVisFull.at<cv::Vec3b>(pnt /mProcessingLevel.scale) = colorRedVec;
            }

            if(cont) continue;

            // draw bounding box
            cv::Point2f p1{comp.start.x, comp.start.y};
            cv::Point2f p2{comp.start.x+comp.size.width, comp.start.y+comp.size.height};
            p1 /= mProcessingLevel.scale; p2 /= mProcessingLevel.scale;
            cv::rectangle(cvVisFull, p1, p2, *color);
            
            // Object &o = mObjects[objInd];

            objInd++;
            std::sprintf(str,"%.2f",comp.iou);
            cv::putText(cvVisFull, str, p1, cv::FONT_HERSHEY_PLAIN, 1, *color);
        }
//        cv::addWeighted(cvVisFull, 0.9, cvObj, 0.1, 0, cvVisFull);

        return mCache.visualizedFull;
    }



}
