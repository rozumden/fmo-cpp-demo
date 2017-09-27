#include "image-util.hpp"
#include <fmo/assert.hpp>
#include <fmo/processing.hpp>
#include <fmo/region.hpp>
#include <iostream>

namespace fmo {
//////////////////////////////////// CIRCLE ////////////////////////////////////////////////////////////////////

    float verifyCircle(const std::vector<cv::Point2f>& pixels, const cv::Point2f &center,
                       const float radius, const float inlierT) {
        int score = 0;
        for (auto &p : pixels) {
            float dist = abs(cv::norm(p - center) - radius);
            if (dist <= inlierT) 
                score += 1 - (dist/inlierT);
        }
        return score;
    }

    float getMaxDist(const std::vector<cv::Point2f>& pixels, const cv::Point2f &center, const float &radius) {
        float maxDist = 0;
        for (auto &p : pixels) {
            float dist = abs(cv::norm(p - center) - radius);
            if(dist > maxDist) maxDist = dist;
        }
        return maxDist;
    }

    inline void getCircle(const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3,
                          cv::Point2f& center, float& radius) {
        float x1 = p1.x;
        float x2 = p2.x;
        float x3 = p3.x;

        float y1 = p1.y;
        float y2 = p2.y;
        float y3 = p3.y;

        center.x = (x1*x1+y1*y1)*(y2-y3) + (x2*x2+y2*y2)*(y3-y1) + (x3*x3+y3*y3)*(y1-y2);
        center.x /= ( 2*(x1*(y2-y3) - y1*(x2-x3) + x2*y3 - x3*y2) );

        center.y = (x1*x1 + y1*y1)*(x3-x2) + (x2*x2+y2*y2)*(x1-x3) + (x3*x3 + y3*y3)*(x2-x1);
        center.y /= ( 2*(x1*(y2-y3) - y1*(x2-x3) + x2*y3 - x3*y2) );

        radius = sqrt((center.x-x1)*(center.x-x1) + (center.y-y1)*(center.y-y1));
    }



    std::vector<cv::Point2f> getPointPositions(cv::Mat binaryImage) {
        std::vector<cv::Point2f> pointPositions;

        for(int y=0; y<binaryImage.rows; ++y) {
            //unsigned char* rowPtr = binaryImage.ptr<unsigned char>(y);
            for(int x=0; x<binaryImage.cols; ++x) {
                //if(rowPtr[x] > 0) pointPositions.push_back(cv::Point2f(x,y));
                if(binaryImage.at<char>(y,x) > 0) pointPositions.emplace_back(cv::Point2f(x,y));
            }
        }

        return pointPositions;
    }

    float findcircle(const std::vector<cv::Point2f>& pixels, const float fmoRadius, SCircle &circle) {
        cv::Point2f bestCircleCenter;
        float bestCircleRadius = -1;
        float inlierT = std::max(1.f,0.1f*fmoRadius);
        float bestScore = 0;

        int maxNrOfIterations = 2*pixels.size();   // TODO: adjust this parameter or include some real ransac criteria with inlier/outlier percentages to decide when to stop
        maxNrOfIterations = 4*fmoRadius;
        for(int its=0; its< maxNrOfIterations; ++its) {
            //RANSAC: randomly choose 3 point and create a circle:
            //TODO: choose randomly but more intelligent,
            //so that it is more likely to choose three points of a circle.
            //For example if there are many small circles, it is unlikely to randomly choose 3 points of the same circle.
            unsigned int idx1 = rand()%pixels.size();
            unsigned int idx2 = rand()%pixels.size();
            unsigned int idx3 = rand()%pixels.size();

            // we need 3 different samples:
            if(idx1 == idx2) continue;
            if(idx1 == idx3) continue;
            if(idx3 == idx2) continue;

            // create circle from 3 points:
            cv::Point2f center{0.f,0.f};
            float radius(0.f);
            getCircle(pixels[idx1],pixels[idx2],pixels[idx3],center,radius);
            if (radius < fmoRadius || radius == std::numeric_limits<float>::infinity()) continue;

            //verify or falsify the circle by inlier counting:
            float score = verifyCircle(pixels,center,radius,inlierT);

            // update best circle information if necessary
            if(score > bestScore)
            {
                bestScore = score;
                bestCircleRadius = radius;
                bestCircleCenter = center;
            }

        }
        circle.radius = bestCircleRadius;
        circle.x = bestCircleCenter.x;
        circle.y = bestCircleCenter.y;
        return bestScore;
    }

    float fitcircle(const std::vector<cv::Point2f>& pixels, const float fmoRadius, SCircle &circle) {
        float bestScore = findcircle(pixels,fmoRadius,circle);

        double startDegree2 = 0;
        double endDegree2 = 360;

        double radiusSz = (fmoRadius / circle.radius) * (180.0f/M_PI);
        for (auto &p : pixels) {
            double angle = atan2(p.y - circle.y, p.x - circle.x) * (180.0/M_PI);
            double angle2 = angle + 180;
            if (angle < 0)
                angle += 360;

            if (angle > circle.startDegree)
                circle.startDegree = angle;
            if (angle < circle.endDegree)
                circle.endDegree = angle;

            if (angle2 > startDegree2)
                startDegree2 = angle2;
            if (angle2 < endDegree2)
                endDegree2 = angle2;
        }

        double sz1 = std::abs(circle.endDegree-circle.startDegree);
        double sz2 = std::abs(endDegree2-startDegree2);
        double sz = sz1;

        if (sz2 < sz1 && std::abs(sz1-sz2) > 5) {
            sz = sz2;
            circle.endDegree = startDegree2 - 180;
            circle.startDegree = endDegree2 - 180;
            circle.startDegreeExt = circle.startDegree - radiusSz;
            circle.endDegreeExt = circle.endDegree + radiusSz;
            circle.length = circle.radius*sz2*M_PI/180.0f;
        } else {
            circle.startDegreeExt = circle.startDegree + radiusSz;
            circle.endDegreeExt = circle.endDegree - radiusSz;
            circle.length = circle.radius*sz1*M_PI/180.0f;
        }

        if (circle.radius == 0 || sz > 160) bestScore = 0;
        return bestScore;
    }

    float fitcircle(const std::vector<cv::Point2f>& pixels, const float fmoRadius, SCircle &circle,
                    const cv::Vec2d &c1, const cv::Vec2d &c2) {
        float bestScore = findcircle(pixels,fmoRadius,circle);

        double angle = atan2(c1[1] - circle.y, c1[0] - circle.x) * (180.0/M_PI);
        double angle2 = atan2(c2[1] - circle.y, c2[0] - circle.x) * (180.0/M_PI);

        int sz = std::abs(angle2 - angle);

        circle.startDegree = angle;
        circle.endDegree = angle2;

        circle.startDegreeExt = circle.startDegree;
        circle.endDegreeExt = circle.endDegree;
        circle.length = circle.radius*sz*M_PI/180.0f;

        if (circle.radius == 0 || sz > 160) bestScore = 0;
        return bestScore;
    }

//////////////////////////////// LINE ////////////////////////////////////////////////////////////////////
    float distanceToLine(const cv::Vec2f &line_start, const cv::Vec2f &line_end, const cv::Vec2f &point) {
        float normalLength = hypot(line_end[0] - line_start[0], line_end[1] - line_start[1]);
        float distance = (float)((point[0] - line_start[0]) * (line_end[1] - line_start[1]) - (point[1] - line_start[1]) * (line_end[0] - line_start[0])) / normalLength;
        return std::abs(distance);
    }

    float distanceToLineVec(const cv::Vec2f &start, const cv::Vec2f &normal, const cv::Vec2f &point) {
        float tt = (start - point).dot(normal);
        return norm((start - point) - tt*normal);
    }
    
    float fitline(const std::vector<cv::Point2f>& pixels, const float radius, SLine &line) {
        float inlierT = std::max(1.f,0.2f*radius);
        cv::fitLine(pixels, line.params, CV_DIST_L2, 0, 0.1, 0.1);
        line.normal.x = line.params[0];
        line.normal.y = line.params[1];
        line.normal = line.normal / norm(line.normal);
        
        line.perp.x = line.normal.y; 
        line.perp.y = -line.normal.x;

        line.start.x = line.params[2];
        line.start.y = line.params[3];
        line.end = line.start + 5*radius*line.normal;

        cv::Point2f origin = line.start + 1000*line.normal;

        float minDist = std::numeric_limits<float>::infinity();
        float maxDist = 0;
        float score = 0;

        for (auto& p : pixels) {
            float e = distanceToLineVec(line.start,line.normal,cv::Vec2f{p.x,p.y});
            if (e <= inlierT) 
                score += 1 - (e/inlierT);

            cv::Point2f p2 = p + e*line.perp;
            float dist = norm(origin - p2);
            if(dist > maxDist) {
                maxDist = dist;
                line.end = p2;
            } 
            if(dist < minDist) {
                minDist = dist;
                line.start = p2;
            }
        }
        line.length = cv::norm(line.start - line.end);
        line.startExt = line.start + radius*line.normal;
        line.endExt = line.end - radius*line.normal;

        return score;
    }

    const void SLine::drawExt(cv::Mat& cvVis, cv::Scalar clr, float thickness) const {
        if(norm(clr) == 0) clr = cv::Scalar{255,0,255};
        if (abs(start.x)+abs(start.y) != 0) {
            cv::line(cvVis, startExt/scale - shift, endExt/scale - shift, clr, thickness);
        }
    }

    const void SLine::draw(cv::Mat& cvVis, cv::Scalar clr, float thickness) const {
        if(norm(clr) == 0) clr = cv::Scalar{255,0,255};
        if (abs(start.x)+abs(start.y) != 0) {
            cv::line(cvVis, start/scale - shift, end/scale - shift, clr, thickness);
        }
    }

    const void SCircle::drawExt(cv::Mat& cvVis, cv::Scalar clr, float thickness) const { 
        if(norm(clr) == 0) clr = cv::Scalar{0,255,255};
        if(radius > 0) {
            cv::Size sz{(int)std::round(radius/scale),(int)std::round(radius/scale)};
            cv::Point2f cntr{x,y};
            cv::ellipse(cvVis, cntr/scale - shift, sz, 0, startDegreeExt, endDegreeExt, clr, thickness);
        }
    }

    const void SCircle::draw(cv::Mat& cvVis, cv::Scalar clr, float thickness) const { 
        if(norm(clr) == 0) clr = cv::Scalar{0,255,255};
        if(radius > 0) {
            cv::Size sz{(int)std::round(radius/scale),(int)std::round(radius/scale)};
            cv::Point2f cntr{x,y};
            cv::ellipse(cvVis, cntr/scale - shift, sz, 0, startDegree, endDegree, clr, thickness);
        }
    }

    // with alpha

    const void SCurve::drawExt(cv::Mat& cvVis, cv::Scalar clr, float thickness, float alpha) const {
        cv::Mat temp = cv::Mat::zeros(cvVis.size(), cvVis.type());
        this->draw(temp,clr,thickness);
        cv::Mat alphaMat = (1-alpha)*(temp > 0)/255 + (temp == 0)/255;
        cvVis = cvVis.mul(alphaMat) + alpha*temp;
    };
}