#include "autoMode.h"

void autoMode::init()
{
    detectioNet = cv::dnn::readNetFromONNX(v8_model_path_);
    clsNet = cv::dnn::readNetFromONNX(cls_model_path_);
}

void autoMode::setModelPath(const std::string &detectio_path)
{
    detectioNet = cv::dnn::readNetFromONNX(detectio_path);
    if (detectioNet.empty())
    {
        emit statusMessageUpdate("模型加载失败");
    }
    else
    {
        emit statusMessageUpdate("模型修改成功");
    }
}

void autoMode::using_once()
{
    std::call_once(flag_, [this](){this->init();});
}

// 预处理
dataImg autoMode::preprocess(cv::Mat &img)
{
    if (img.empty())
    {
        std::cout<<"lab is empty"<<std::endl;
        return dataImg();
    }

    // 0.调整亮度
    cv::Mat lab;
    cvtColor(img, lab, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> channels;
    split(lab, channels);

    double l_mean = mean(channels[0])[0];
    if (l_mean < 10)
    {
        cv::Mat lut(1, 256, CV_8UC1);
        for (int i = 0; i < 256; i++)
        {
            lut.at<uchar>(i) = cv::saturate_cast<uchar>(pow(i / 255.0, 0.7) * 255.0);
        }
        LUT(channels[0], lut, channels[0]);
    }

    cv::Mat bgr_img;
    cv::merge(channels, lab);
    cvtColor(lab, bgr_img, cv::COLOR_Lab2BGR);

    // 1.获取图像尺寸
    int h = bgr_img.rows;
    int w = bgr_img.cols;
    int th = target_size;
    int tw = target_size;

    // 2.计算缩放比例
    float scale = std::min(static_cast<float>(tw) / w, static_cast<float>(th) / h);
    scale = std::max(scale, 0.01f);
    int new_w = static_cast<int>(w * scale);
    int new_h = static_cast<int>(h * scale);

    // 3.缩放图像
    cv::Mat resized_img;
    resize(bgr_img, resized_img, cv::Size(new_w, new_h));

    // 4.计算填充量
    int padW = tw - new_w;
    int padH = th - new_h;

    int left = padW / 2;
    int right = padW - left;
    int top = padH / 2;
    int bottom = padH - top;

    // 5.填充图像
    cv::Mat padded_img;
    copyMakeBorder(resized_img, padded_img, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    // 6.创建blob
    cv::Mat blob = cv::dnn::blobFromImage(padded_img, 1.0/255.0, cv::Size(target_size, target_size), cv::Scalar(0,0,0), true, CV_32F);

    dataImg imgdata;
    imgdata.scale = scale;
    imgdata.padW = padW;
    imgdata.padH = padH;
    imgdata.input = bgr_img.clone();
    imgdata.blob = blob;

    return imgdata;
}

// 主函数 推理和后处理
void autoMode::Inference(cv::Mat &img)
{
    if (detectioNet.empty())
    {
        std::cout << "模型加载失败" << std::endl;
        return;
    }

    qualifiedArmors.clear();
    finalArmors_.clear();

    cvtColor(img, img, cv::COLOR_RGB2BGR);

    dataImg imgdata = preprocess(img);

    detectioNet.setInput(imgdata.blob);
    std::vector<cv::Mat> outputs;
    std::vector<std::string> outNames = detectioNet.getUnconnectedOutLayersNames();
    detectioNet.forward(outputs, outNames);

    cv::Mat output = outputs[0];   // output: 1*5*8400

    // v8格式：1*5*8400（batch_size * 特征维度（4坐标+1置信度）* 检测框数量）
    const int features = output.size[1];
    const int boxes = output.size[2];

    // 重塑为二维矩阵
    cv::Mat outputMat = output.reshape(1, features);  //  5 * 8400

    preArmor preArmor;
    for (int i = 0; i < boxes; i++)
    {
        float confidence = outputMat.at<float>(4, i);

        if (confidence > confidence_threshold_)
        {
            // 获取边界框信息
            float cx = outputMat.at<float>(0, i);
            float cy = outputMat.at<float>(1, i);
            float w = outputMat.at<float>(2, i);
            float h = outputMat.at<float>(3, i);

            // 坐标映射回原始图像
            float cx_unpad = (cx - imgdata.padW / 2) / imgdata.scale;
            float cy_unpad = (cy - imgdata.padH / 2) / imgdata.scale;
            float w_unpad = w / imgdata.scale;
            float h_unpad = h / imgdata.scale;

            // 计算边界框的左上角和宽高
            int lx = static_cast<int>(cx_unpad - w_unpad / 2);
            int ly = static_cast<int>(cy_unpad - h_unpad / 2);
            int width = static_cast<int>(w_unpad);
            int height = static_cast<int>(h_unpad);

            // 边界检查
            if (lx <= 0 || ly <= 0 || lx + width > img.cols || ly + height > img.rows || width <= 5 || height <= 5)
            {
                continue;
            }

            cv::Rect box(lx, ly, width, height);
            preArmor.boxes.push_back(box);
            preArmor.confidence.push_back(confidence);
        }
    }

    // NMS处理
    std::vector<int> indices;
    cv::dnn::NMSBoxes(preArmor.boxes, preArmor.confidence, confidence_threshold_, nms_threshold_, indices);

    for (auto i : indices)
    {
        inferArmor inferArmor;
        inferArmor.box = preArmor.boxes[i];
        inferArmor.confidence = preArmor.confidence[i];

        colorFiliter(img, inferArmor);
    }
    std::cout << "qualifiedArmors: " << qualifiedArmors.size() << std::endl;

    if (is_warp_)
    {
        barFiliter();
        armorFiliter();

        if (!qualifiedArmors.empty() && finalArmors_.empty())
        {
            noWarpFiliter();
        }
    }
    else
    {
        noWarpFiliter();
    }

    if (!finalArmors_.empty())
    {
        classify(img);
        update();
    }
    else
    {
        emit statusMessageUpdate("未检测到目标");
    }
}

// 根据灯条颜色进行筛选
void autoMode::colorFiliter(cv::Mat &img, inferArmor &inferArmor)
{
    if (img.empty() || inferArmor.box.x < 0 || inferArmor.box.y < 0 || inferArmor.box.x + inferArmor.box.width > img.cols ||
        inferArmor.box.y + inferArmor.box.height > img.rows || inferArmor.box.width <= 0 || inferArmor.box.height <= 0)
    {
        std::cout << "Invalid ROI for color filtering" << std::endl;
        return;
    }

    cv::Mat roi = img(inferArmor.box);
    inferArmor.original_roi = roi;

    cv::Mat hsv;
    roi.copyTo(hsv);
    cvtColor(hsv, hsv, cv::COLOR_BGR2HSV);

    cv::Mat red_binary_high, red_binary_low, red_binary, blue_binary;
    // 红色区域
    inRange(hsv, cv::Scalar(red_h_min_low_, red_s_min_, red_v_min_), cv::Scalar(red_h_max_low_, red_s_max_, red_v_max_), red_binary_low);
    inRange(hsv, cv::Scalar(red_h_min_high_, red_s_min_, red_v_min_), cv::Scalar(red_h_max_high_, red_s_max_, red_v_max_), red_binary_high);
    bitwise_or(red_binary_low, red_binary_high, red_binary);
    int redArea = countNonZero(red_binary);
    // 蓝色区域
    inRange(hsv, cv::Scalar(blue_h_min_, blue_s_min_, blue_v_min_), cv::Scalar(blue_h_max_, blue_s_max_, blue_v_max_), blue_binary);
    int blueArea = countNonZero(blue_binary);

    inferArmor.color = redArea > blueArea ? "red" : "blue";
    inferArmor.binary_roi = redArea > blueArea ? red_binary : blue_binary;
    qualifiedArmors.push_back(inferArmor);
}

// 判断灯条是否有效
bool autoMode::isValidBar(Bar &bar)
{
    if (bar.ratio >= max_lw_ratio_ || bar.ratio <= min_lw_ratio_)
    {
        std::cout<<"bar.ratio false"<<std::endl;
        return false;
    }

    if (abs(bar.angle) <= min_bar_angle_)
    {
        std::cout<<"bar.angle false"<<std::endl;
        return false;
    }

    return true;
}

// 标准化灯条四点顺序
void standardizeBar(Bar &bar)
{
    cv::Point2f src_points[4];
    bar.rect.points(src_points);

    cv::Point2f lb, lt, rb, rt;

    std::sort(src_points, src_points + 4, [](const cv::Point2f &a, const cv::Point2f &b) {
        return a.y >= b.y;
    });

    // 下方点中 x 较小的为左下，x 较大的为右下
    if (src_points[0].x < src_points[1].x)
    {
        lb = src_points[0];
        rb = src_points[1];
    }
    else
    {
        lb = src_points[1];
        rb = src_points[0];
    }
    // 上方点中 x 较小的为左上，x 较大的为右上
    if (src_points[2].x < src_points[3].x)
    {
        lt = src_points[2];
        rt = src_points[3];
    }
    else
    {
        lt = src_points[3];
        rt = src_points[2];
    }

    // 计算灯条长度
    double norm_width = std::sqrt(std::pow(rt.x - lt.x, 2) + std::pow(rt.y - lt.y, 2));
    double norm_height = std::sqrt(std::pow(lt.x - lb.x, 2) + std::pow(lt.y - lb.y, 2));

    double long_one = std::max(norm_width, norm_height);

    bar.sorted_points.push_back(lb);
    bar.sorted_points.push_back(lt);
    bar.sorted_points.push_back(rt);
    bar.sorted_points.push_back(rb);
    bar.long_one = long_one;
}

// 定位灯条准确位置
void autoMode::barFiliter()
{
    for (auto &armor : qualifiedArmors)
    {
        cv::Mat roi = armor.binary_roi;
        std::vector<Bar> tmpBars;

        cv::Mat close_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 7));
        morphologyEx(roi, roi, cv::MORPH_CLOSE, close_kernel);

        std::vector<std::vector<cv::Point>> contours;
        findContours(roi, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (auto &contour : contours)
        {
            if (contourArea(contour) < 20)
                continue;

            cv::RotatedRect rect = minAreaRect(contour);

            cv::Point2f vertices[4];
            rect.points(vertices);

            float angle;
            float width = rect.size.width;
            float height = rect.size.height;
            float max_len = std::max(width, height);
            float ratio = max_len / std::min(width, height);

            if (max_len == height)
                angle = rect.angle + 90.0f;
            else
                angle = rect.angle;

            Bar tmpbar;
            tmpbar.rect = rect;
            tmpbar.center = rect.center;
            tmpbar.ratio = ratio;
            tmpbar.angle = angle;

            if (isValidBar(tmpbar))
            {
                tmpBars.push_back(tmpbar);
            }
        }

        std::sort(tmpBars.begin(), tmpBars.end(), [](const Bar& a, const Bar& b) {
            return a.center.x < b.center.x;
        });

        for (auto &tmpBar : tmpBars)
        {
            standardizeBar(tmpBar);
        }

        armor.bars = tmpBars;
    }
}

// 判断组合成装甲板的灯条
bool autoMode::isValidArmor(inferArmor &armor)
{
    // 灯条两两匹配
    std::vector<Bar> bars = armor.bars;
    int match[2];

    auto pairArmor = [this](std::vector<Bar> &bars, int match[2])
    {
        for (int i = 0; i < bars.size() - 1; i++)
        {
            for (int j = i + 1; j < bars.size(); j++)
            {
                Bar left_bar = bars[i];
                Bar right_bar = bars[j];

                double distance = std::sqrt(std::pow(left_bar.center.x - right_bar.center.x, 2) + std::pow(left_bar.center.y - right_bar.center.y, 2));

                double bars_length = left_bar.long_one + right_bar.long_one;
                double ratio = std::max(left_bar.long_one, right_bar.long_one) / std::min(left_bar.long_one, right_bar.long_one);

                double raw_angle_diff = fabs(left_bar.angle - right_bar.angle);
                double angle = fmod(raw_angle_diff, 180);
                if (angle > 90)
                    angle = 180 - angle;

                if (distance <= bars_length * min_bars_distance_ || distance >= bars_length * max_bars_distance_)
                {
                    continue;
                }

                if (ratio > max_bars_ratio_)
                {
                    continue;
                }

                if (angle > max_bars_angle_)
                {
                    continue;
                }

                match[0] = i;
                match[1] = j;

                return true;
            }
        }
        return false;
    };

    if (pairArmor(bars, match))
    {
        armor.bars.clear();
        armor.bars.push_back(bars[match[0]]);
        armor.bars.push_back(bars[match[1]]);
        return true;
    }

    return false;
}

// 定位装甲板准确位置
void autoMode::armorFiliter()
{
    for (auto &armor : qualifiedArmors)
    {
        if (armor.bars.empty())
        {
            std::cout<<"No bars found!"<<std::endl;
            continue;
        }

        if (!isValidArmor(armor))
            continue;

        std::vector<Bar> bars = armor.bars;

        // 计算装甲板四顶点
        std::vector<cv::Point2f> left(4);            // lb, lt, rt, rb
        std::vector<cv::Point2f> right(4);
        std::vector<cv::Point2f> armor_points(4);    // lb, lt, rt, rb

        // 转换为全局坐标系
        for (auto &bar : bars)
        {
            for (auto &point : bar.sorted_points)
            {
                point += cv::Point2f(armor.box.x, armor.box.y);
            }

            bar.center += cv::Point2f(armor.box.x, armor.box.y);
        }

        left = bars[0].sorted_points;
        right = bars[1].sorted_points;

        armor_points[0] = (left[0] + left[3]) * 0.5;
        armor_points[1] = (left[1] + left[2]) * 0.5;
        armor_points[2] = (right[1] + right[2]) * 0.5;
        armor_points[3] = (right[0] + right[3]) * 0.5;

        // 存储最终结果
        finalArmor final_armor;
        final_armor.box = armor.box;
        final_armor.armor_points = armor_points;
        final_armor.color = armor.color;
        final_armor.confidence = armor.confidence;

        finalArmors_.push_back(final_armor);
    }
    std::cout << "finalArmors_: " << finalArmors_.size() << std::endl;
}

void autoMode::noWarpFiliter()
{
    for (auto &armor : qualifiedArmors)
    {
        if (armor.box.empty())
        {
            return;
        }

        finalArmor final_armor;
        final_armor.box = armor.box;
        final_armor.color = armor.color;
        final_armor.confidence = armor.confidence;

        finalArmors_.push_back(final_armor);
    }
}

// 透视变换
cv::Mat warp(cv::Mat &img, finalArmor &armor)
{
    cv::Mat display = img.clone();
    for (int i = 0; i < 4; i++)
    {
        circle(display, armor.armor_points[i], 5, cv::Scalar(0, 255, 0), -1);
    }

    const int top_light_y_ = (warp_height_ - light_len_) / 2;
    const int bottom_light_y_ = top_light_y_ + light_len_;

    cv::Point2f src_pts[4] = {armor.armor_points[0], armor.armor_points[1], armor.armor_points[2], armor.armor_points[3]};

    cv::Point2f dst_pts[4] = {
        cv::Point2f(0, bottom_light_y_),
        cv::Point2f(0, top_light_y_),
        cv::Point2f(warp_width_, top_light_y_),
        cv::Point2f(warp_width_, bottom_light_y_),
    };

    cv::Mat warp_img;
    auto warp_mat = getPerspectiveTransform(src_pts, dst_pts);
    try
    {
        warpPerspective(img, warp_img, warp_mat, cv::Size(warp_width_, warp_height_), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));
    }
    catch (const cv::Exception &e)
    {
        warp_img = cv::Mat();
    }

    return warp_img;
}

// 提取数字图像
cv::Mat autoMode::extractNumber(cv::Mat &img, finalArmor &armor)
{
    cv::Mat num_img;
    if (is_warp_)
    {
        num_img = warp(img, armor);
        if (num_img.empty())
        {
            num_img = img(armor.box);
        }
    }
    else
    {
        num_img = img(armor.box);
    }

    int src_w = num_img.cols;
    int src_h = num_img.rows;
    int crop_x = static_cast<int>(src_w * 0.2);
    int crop_y = 0;
    int crop_w = static_cast<int>(src_w * 0.65);
    int crop_h = src_h;

    crop_x = std::max(0, crop_x);
    crop_w = std::min(crop_w, src_w - crop_x);

    cv::Rect roi(crop_x, crop_y, crop_w, crop_h);
    cv::Mat crop_img = num_img(roi);

    resize(crop_img, crop_img, cv::Size(roi_w_, roi_h_));

    if (crop_img.empty())
    {
        std::cout<<"crop_img is empty"<<std::endl;
        return crop_img;
    }

    armor.warp = crop_img.clone();

    // 二值化处理
    cvtColor(crop_img, crop_img, cv::COLOR_BGR2GRAY);
    threshold(crop_img, crop_img, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    return crop_img;
}

// 数字分类
void autoMode::classify(cv::Mat img)
{
    for (auto &armor : finalArmors_)
    {
        cv::Mat num_mat, num_blob;
        num_mat = extractNumber(img, armor);

        cv::dnn::blobFromImage(num_mat, num_blob);
        clsNet.setInput(num_blob);
        cv::Mat outputs = clsNet.forward();

        // softmax处理
        float max_prob = *std::max_element(outputs.begin<float>(), outputs.end<float>());
        cv::Mat softmax_prob;
        cv::exp(outputs - max_prob, softmax_prob);
        float sum = static_cast<float>(cv::sum(softmax_prob)[0]);
        softmax_prob /= sum;

        // 获取分类结果
        double confidence;
        cv::Point class_id_point;
        minMaxLoc(softmax_prob.reshape(1, 1), nullptr, &confidence, nullptr, &class_id_point);
        int label_id = class_id_point.x;

        // 更新装甲板信息
        std::string label = class_names_[label_id];
        armor.label = label;
    }
}

void autoMode::update()
{
    if (labelMode_ == "cls")
    {
        for (auto &armor : finalArmors_)
        {
            detectionLabel label;
            label.name = armor.label;
            label.rect = armor.box;
            label.is_selected = false;
            label.is_saved = false;

            if (is_warp_)
            {
                label.warp = armor.warp;
            }
            else
            {
                label.warp = NULL;
            }

            detectionLabels_.push_back(label);
        }
    }
}