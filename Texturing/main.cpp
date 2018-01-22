#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include <iomanip>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

cv::Mat DrawPyramid(std::vector<cv::Mat> Pyramid) {
    cv::Mat result = cv::Mat(Pyramid[0].size(),
                             Pyramid[0].type());


    int currentSize = Pyramid[0].rows;
    int startPointX = 0;
    int startPointY = 0;
    for (int i = 1; i < Pyramid.size(); ++i) {
        currentSize /= 2;

        Pyramid[i].copyTo(result(cv::Rect(startPointX, startPointY, currentSize, currentSize)));

        if (i % 2 == 1) {
            startPointX += currentSize;
        } else {
            startPointY += currentSize;
        }

    }

    return result;
}

std::vector<cv::Mat> GetGaussianPyramid(cv::Mat input, int numLevels) {
    std::vector<cv::Mat> result;
    result.push_back(input);

    for (int i = 0; i < numLevels; ++i) {
        cv::Mat downsampled;
        cv::pyrDown(result.back(), downsampled);
        result.push_back(downsampled);
    }

    return result;
}

std::vector<cv::Mat> GetLaplacianPyramid(cv::Mat input, int numLevels) {
    std::vector<cv::Mat> result;

    cv::Mat lastPyramidLevel = input;

    for (int i = 0; i < numLevels; ++i) {

        cv::Mat downsampled;
        cv::pyrDown(lastPyramidLevel, downsampled);

        cv::Mat upsampled;
        cv::pyrUp(downsampled, upsampled);

        lastPyramidLevel.convertTo(lastPyramidLevel, CV_32FC3, 1.f/255.f);
        upsampled.convertTo(upsampled, CV_32FC3, 1.f/255.f);

        cv::Mat laplace;
        cv::subtract(lastPyramidLevel, upsampled, laplace);

        result.push_back(laplace);

        lastPyramidLevel = downsampled;
    }

    lastPyramidLevel.convertTo(lastPyramidLevel, CV_32FC3, 1.f/255.f);
    result.push_back(lastPyramidLevel);

    return result;
}

std::vector<cv::Mat>
BlendPyramidLevels(std::vector<std::vector<cv::Mat> > LaplacianPyramids,
                   std::vector<std::vector<cv::Mat> > MaskGaussianPyramids) {

    std::vector<cv::Mat> blendedPyramid;

    int numLevels = LaplacianPyramids[0].size();
    for (int i = 0; i < numLevels; ++i) {

        std::cout << "Blending Level " << i << std::endl;

        std::vector<cv::Mat> imagesToBlendAtCurrentLevel;
        for (auto laplacianPyramid : LaplacianPyramids) {
            auto imageToBlend = laplacianPyramid[i];
            imagesToBlendAtCurrentLevel.push_back(imageToBlend);
        }

        std::vector<cv::Mat> alphaMasksAtCurrentLevel;
        for (auto maskGaussianPyramid : MaskGaussianPyramids) {
            auto alphaMaskAtCurrentLevel = maskGaussianPyramid[i];
            alphaMaskAtCurrentLevel.convertTo(alphaMaskAtCurrentLevel, CV_32FC1, 1.f/255.f);
            alphaMaskAtCurrentLevel = alphaMaskAtCurrentLevel.mul(alphaMaskAtCurrentLevel);
            alphaMaskAtCurrentLevel = alphaMaskAtCurrentLevel.mul(alphaMaskAtCurrentLevel);
            // alphaMaskAtCurrentLevel = alphaMaskAtCurrentLevel.mul(alphaMaskAtCurrentLevel);
            cv::Mat temp3Channel[] = {alphaMaskAtCurrentLevel, alphaMaskAtCurrentLevel, alphaMaskAtCurrentLevel};

            cv::Mat alpha3Channel;
            cv::merge(temp3Channel, 3, alpha3Channel);

            alphaMasksAtCurrentLevel.push_back(alpha3Channel);
        }

        cv::Mat summedAlpha = cv::Mat::zeros(alphaMasksAtCurrentLevel[0].size(),
                                             alphaMasksAtCurrentLevel[0].type());

        for (auto mask : alphaMasksAtCurrentLevel) summedAlpha += mask;
        for (auto mask : alphaMasksAtCurrentLevel) mask = mask.mul(1 / summedAlpha);



#if 0
        for (int i = 0; i < imagesToBlendAtCurrentLevel.size(); ++i) {
            std::stringstream sb;
            sb << "image_" << i << ".jpg";
            cv::imwrite(sb.str(), imagesToBlendAtCurrentLevel[i] * 255);
            //cv::imshow(sb.str() , imagesToBlendAtCurrentLevel[i]);
        }

#endif

        cv::Mat blended = cv::Mat::zeros(imagesToBlendAtCurrentLevel[0].size(),
                                         imagesToBlendAtCurrentLevel[0].type());

        for (int j = 0; j < imagesToBlendAtCurrentLevel.size(); ++j) {
            // cv::Mat A = blended.mul(1 - alphaMasksAtCurrentLevel[j]);
            cv::Mat mul = imagesToBlendAtCurrentLevel[j].mul(alphaMasksAtCurrentLevel[j]);
            blended += mul;

            std::stringstream sb;
            sb << "alpha_level_" << i << "_image_" << j << ".png";
            cv::imwrite(sb.str(), 255 * alphaMasksAtCurrentLevel[j]);

            sb.str("");
            sb.clear();
            sb << "blend_level_" << i << "_image_" << j << ".png";
            cv::imwrite(sb.str(), 255 * imagesToBlendAtCurrentLevel[j]);
        }

        blendedPyramid.push_back(blended);
    }

    return blendedPyramid;
}

cv::Mat StandardAlphaBlend(std::vector<cv::Mat> imagesToBlend,
                           std::vector<cv::Mat> alphaValues)
{
    std::vector<cv::Mat> imagesToBlendFloat;
    for (auto image : imagesToBlend) {
        image.convertTo(image, CV_32FC3, 1.f/255.f);

        cv::imshow("Image", image);
        cv::waitKey(0);

        imagesToBlendFloat.push_back(image);
    }

    cv::imwrite("rgb1.png", 255 * imagesToBlendFloat[0]);
    cv::imwrite("rgb2.png", 255 * imagesToBlendFloat[1]);
   // cv::imwrite("rgb3.png", 255 * imagesToBlendFloat[2]);

    std::vector<cv::Mat> alphaMasksFloat;
    for (auto alpha : alphaValues) {
        alpha.convertTo(alpha, CV_32FC1, 1.f/255.f);

        alpha = alpha.mul(alpha);
        alpha = alpha.mul(alpha);

        cv::Mat temp3Channel[] = {alpha, alpha, alpha};

        cv::Mat alpha3Channel;
        cv::merge(temp3Channel, 3, alpha3Channel);

        cv::imshow("Mask", alpha3Channel);
        cv::waitKey(0);
        alphaMasksFloat.push_back(alpha3Channel);
    }

    cv::Mat summedAlpha = cv::Mat::zeros(alphaMasksFloat[0].size(),
                                         alphaMasksFloat[0].type());
    for (auto alpha : alphaMasksFloat) summedAlpha += alpha;
    for (auto alpha : alphaMasksFloat) alpha = alpha.mul(1 / summedAlpha);

    cv::imwrite("alpha1.png", 255 * alphaMasksFloat[0]);
    cv::imwrite("alpha2.png", 255 * alphaMasksFloat[1]);
    //cv::imwrite("alpha3.png", 255 * alphaMasksFloat[2]);


    std::vector<cv::Mat> multiplied;
    for (int i = 0; i < imagesToBlend.size(); ++i) {
        multiplied.push_back(imagesToBlendFloat[i].mul(alphaMasksFloat[i]));
    }

    cv::imwrite("mul_alpha1.png", 255 * multiplied[0]);
    cv::imwrite("mul_alpha2.png", 255 * multiplied[1]);
    // cv::imwrite("mul_alpha3.png", 255 * multiplied[2]);

    cv::Mat blended = cv::Mat::zeros(imagesToBlendFloat[0].size(),
                                     imagesToBlendFloat[0].type());

    for (int i = 0; i < imagesToBlend.size(); ++i) {
        blended += multiplied[i];
    }

    return blended;
}

int main(int argc, char** argv) {

    int numPyramidLevels = 6;
    if (argc > 1) {
        numPyramidLevels = std::atoi(argv[1]);
    }

    std::vector<cv::Mat> imagesToBlend;
    std::vector<cv::Mat> alphaValues;

#if 1

    //
    // Face-Textures (alphas are stored in the images)
    //

    cv::Mat image1 = cv::imread("..\\..\\data\\002_test_texture.png", cv::IMREAD_UNCHANGED);
    cv::Mat image2 = cv::imread("..\\..\\data\\003_test_texture.png", cv::IMREAD_UNCHANGED);
    cv::Mat image3 = cv::imread("..\\..\\data\\004_test_texture.png", cv::IMREAD_UNCHANGED);

    std::vector<cv::Mat> images; images.push_back(image1); images.push_back(image2); images.push_back(image3);

    for (auto mat : images) {
        std::vector<cv::Mat> channels;
        cv::split(mat, channels);

        cv::Mat blendImage;
        cv::merge(&channels[0], 3, blendImage);

        cv::Mat alphaMask = 255 - channels[3].clone();

        imagesToBlend.push_back(blendImage);
        alphaValues.push_back(alphaMask);
    }

#else

    //
    // Apple orange part (seperate alpha)
    //

    // cv::Mat image1 = cv::imread("..\\..\\data\\apple.jpg", cv::IMREAD_UNCHANGED);
    // cv::Mat image2 = cv::imread("..\\..\\data\\orange.jpg", cv::IMREAD_UNCHANGED);

    cv::Mat image1 = cv::imread("..\\..\\data\\blend_jonas.jpg");
    cv::Mat image2 = cv::imread("..\\..\\data\\blend_stephan.jpg");
    cv::Mat image3 = cv::imread("..\\..\\data\\blend_findan.jpg");
    cv::Mat image4 = cv::imread("..\\..\\data\\blend_harald.jpg");


    imagesToBlend.push_back(image2);
    imagesToBlend.push_back(image3);
    imagesToBlend.push_back(image4);
    imagesToBlend.push_back(image1);

    int cols = image1.cols;
    int rows = image1.rows;

    cv::Mat mask = 255 * cv::Mat::ones(rows / 2, cols / 2, CV_8UC1);

    cv::Mat alpha = cv::Mat::zeros(rows, cols, CV_8UC1);
    cv::Rect destRect = cv::Rect(cv::Point(0, 0), mask.size());
    mask.copyTo(alpha(cv::Rect(destRect)));

    alphaValues.push_back(alpha);

    cv::Mat alpha1;
    cv::flip(alpha, alpha1, 1);

    cv::Mat alpha2;
    cv::flip(alpha1, alpha2, 0);

    cv::Mat alpha3;
    cv::flip(alpha2, alpha3, 1);

    // alphaValues.push_back(alpha);
    // cv::Mat rightAlpha = 255 - alpha;
    // alphaValues.push_back(rightAlpha);

    alphaValues.push_back(alpha1);
    alphaValues.push_back(alpha2);
    alphaValues.push_back(alpha3);




    cv::imshow("alpha1", alpha);
    cv::imshow("alpha2", alpha1);
    cv::imshow("alpha3", alpha2);
    cv::imshow("alpha4", alpha3);
    cv::waitKey(0);

#endif


#if 0
    //
    // Alpha Blend
    //

    cv::Mat result = StandardAlphaBlend(imagesToBlend, alphaValues);

    cv::imwrite("AlphaBlend.png", 255 * result);
    return 1;
#endif

    //
    // Pyramid Blend
    //

    std::cout << "Generating Laplacian Pyramids" << std::endl;
    std::vector<std::vector<cv::Mat> > LaplacianPyramids;

    for (auto image : imagesToBlend) {
        LaplacianPyramids.push_back(GetLaplacianPyramid(image, numPyramidLevels));
    }

    for (int i = 0; i < imagesToBlend.size(); ++i) {
        std::stringstream sb;
        sb << "Input_" << i << ".png";

        cv::imwrite(sb.str(), imagesToBlend[i]);

        sb.str("");
        sb.clear();
        sb << "Alpha_" << i << ".png";

        cv::imwrite(sb.str(), alphaValues[i]);

    }

#if 0
    int count = 0;
    for (auto Pyramid : LaplacianPyramids) {
        cv::Mat drawnPyramid = DrawPyramid(Pyramid);

        std::stringstream sb;
        sb << "Laplace_" << count++ << ".jpg";

        cv::imwrite(sb.str(), drawnPyramid);
    }
#endif

    std::cout << "Generating Gaussian Pyramids for Masks" << std::endl;
    std::vector<std::vector<cv::Mat> > MaskGaussianPyramids;
    for (auto alphaMask : alphaValues) {
        MaskGaussianPyramids.push_back(GetGaussianPyramid(alphaMask, numPyramidLevels));
    }

    std::cout << "Blending Pyramids" << std::endl;

    std::vector<cv::Mat> blendedPyramid = BlendPyramidLevels(LaplacianPyramids, MaskGaussianPyramids);

    cv::Mat drawnPyramid = DrawPyramid(blendedPyramid);
    cv::imwrite("BlendPyramid.jpg", 255 * drawnPyramid);

#if 0
    for (auto mat : blendedPyramid) {
        std::cout << "(" << mat.cols <<" x " << mat.rows << ")" << std::endl;
        cv::imshow("Blend at level", mat);
        cv::waitKey(0);
    }
#endif

    // TODO: CHECK THIS

    int last = blendedPyramid.size() - 1;
    cv::Mat collapsed = blendedPyramid[last];

    for (int i = last - 1; i >= 0; --i) {
        cv::imshow("Expanding", collapsed);
        cv::waitKey(0);
        cv::pyrUp(collapsed, collapsed);
        cv::add(collapsed, blendedPyramid[i], collapsed);
    }

    cv::imshow("PyramidBlend", collapsed);
    cv::imwrite("PyramidBlend.png", 255 * collapsed);
    cv::waitKey(0);
}

#if 0
                if (inTriangle) {
                    cv::Point2f samplePoint = cv::Point2f(p1.x, p1.y);
#if 1
                    cv::Vec3f samplePoint3D = cv::Vec3f(v1.x, v1.y, v1.z) +
                                                          u * v1v2 +
                                                          v * v1v3;

                    // TODO: Is this perspectively correct?

                    cv::Vec3f v1Vec = v1;
                    cv::Vec3f v2Vec = v2;
                    cv::Vec3f v3Vec = v3;
                    samplePoint3D = (1.0f - u - v) * v1Vec / v1.z +
                                                u  * v2Vec / v2.z +
                                                v  * v3Vec / v3.z;

                    float z = 1 / ((1.0f - u - v) * v1.z +
                                                u * v2.z +
                                                v * v3.z);
                    samplePoint3D /= z;

                    CameraSpacePoint samplePoint3DCSP = {samplePoint3D[0], samplePoint3D[1], samplePoint3D[2]};
                    ColorSpacePoint  samplePoint2DCSP;
                    hr = coordinateMapper->MapCameraPointToColorSpace(samplePoint3DCSP, &samplePoint2DCSP);
                    if (SUCCEEDED(hr) && !std::isinf(samplePoint2DCSP.X) && !std::isinf(samplePoint2DCSP.Y) &&
                        samplePoint2DCSP.X > 0 && samplePoint2DCSP.Y > 0 &&
                        samplePoint2DCSP.X < 1920 && samplePoint2DCSP.Y < 1079) {
                    }
                    else {
                        numBadMappings++;
                        numBadMappingsFromInterp++;
                        continue;
                    }
                    samplePoint.x = samplePoint2DCSP.X;
                    samplePoint.y = samplePoint2DCSP.Y;


#else


                    // Perspective incorrect interpolation
      //              samplePoint = cv::Vec2f(p1.x, p1.y) + u * p1p2 + v * p1p3;

                    // Perspective correct
                    // v1 maps to p1
                    // v2 maps to p2
                    // v3 maps to p3
                    samplePoint = cv::Vec2f(p1.x, p1.y) + u * p1p2 + v * p1p3;
                    //samplePoint.x /= (v1.z + u * v1v2[2] + v * v1v3[2]);
                    //samplePoint.y /= (v1.z + u * v1v2[2] + v * v1v3[2]);

#endif
#endif
