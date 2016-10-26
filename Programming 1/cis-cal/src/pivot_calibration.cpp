//
// Created by gaddra on 10/26/16.
//

#include "pivot_calibration.h"

Eigen::Matrix<double, 6, 1>
cis::pivot_calibration(const std::vector<cis::PointCloud> &frames) {
    // The first frame is moved to the origin
    const auto reference_frame = frames.at(0).center();

    // {{R_0,-I},...,{R_N,-I}}
    Eigen::Matrix<double, Eigen::Dynamic, 6> A;
    A.resize(3 * frames.size(), Eigen::NoChange);
    Eigen::Matrix<double, Eigen::Dynamic, 1> b;
    b.resize(3 * frames.size(), Eigen::NoChange);
    const Eigen::Matrix<double, 3, 3> neg_ident = -Eigen::Matrix<double, 3, 3>::Identity();

    // Compute the transformation and build A, b matricies
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto trans = cloud_to_cloud(reference_frame, frames.at(i));
        A.block(3 * i, 0, 3, 3) = trans.rotation().matrix();
        A.block(3 * i, 3, 3, 3) = neg_ident;
        b.block(3 * i, 0, 3, 1) = -trans.translation().matrix(); //Needs to be the negative translation
    }

    // Solve the system
    return A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
}

Eigen::Matrix<double,6,1>
cis::pivot_calibration(const std::vector<cis::PointCloud> &frames,
                  const Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> &fn) {

    const Point
            _min = min(frames),
            _max = max(frames);

    std::vector<cis::PointCloud> calibrated(0);
    // Calibrate all the points
    Point cal_pt;
    for (const auto &frame : frames) {
        calibrated.emplace_back();
        auto &back = calibrated.back();
        for (size_t k = 0; k < frame.size(); ++k) {
            cal_pt = scale_to_box(frame.at(k), _min, _max);
            cal_pt = interpolation_poly<5>(cal_pt) * fn;
            back.add_point(cal_pt);
        }
    }

    return pivot_calibration(calibrated);
}

Eigen::Matrix<double, 6, 1>
cis::pivot_calibration_opt(const std::vector<cis::PointCloud> &opt, const std::vector<cis::PointCloud> &em) {
    if (opt.size() != em.size()) throw std::invalid_argument("Both vectors must be the same size.");
    // The first frame is moved to the origin
    const PointCloud em_reference_frame = em.at(0).center();
    std::vector<PointCloud> trans_opt(opt.size());
    Eigen::Matrix<double, 4, 4> transmat;
    for (int j = 0; j < opt.size(); ++j) {
        const auto trans = cloud_to_cloud(em_reference_frame, em.at(j));
        transmat.block(0,0,3,3) = trans.rotation().matrix().inverse();
        transmat.block(0,3,3,1) = -trans.rotation().matrix().inverse() * trans.translation();
        transmat.block(3,0,1,4) = Eigen::Matrix<double,1,4>{0,0,0,1};
        PointCloud n;
        for (int i = 0; i < opt.at(j).size(); ++i) {
            n.add_point((transmat * opt.at(j).at(i).homogeneous()).block(0,0,3,1));
        }
        trans_opt[j] = n;
    }
    return pivot_calibration(trans_opt);
}