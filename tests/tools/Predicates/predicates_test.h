/* This file is part of PyMesh. Copyright (c) 2017 by Qingnan Zhou */
#pragma once

#include <TestBase.h>
extern "C" {
#include <Predicates/predicates.h>
}

TEST(Orientat3D, Simple) {
    double a[3]{0.0, 0.0, 0.0};
    double b[3]{0.0, 0.0, 0.0};
    double c[3]{0.0, 0.0, 0.0};
    double d[3]{0.0, 0.0, 0.0};
    double r1 = prdc_orient3d(a,b,c,d);
    double r2 = prdc_orient3dexact(a,b,c,d);
    using Arr3D = Eigen::Matrix<double, 3, 1>;
    auto run = [](Arr3D& pa, Arr3D& pb, Arr3D& pc, Arr3D& pd) {
        return prdc_orient3d(pa.data(), pb.data(), pc.data(), pd.data());
    };
    Arr3D pa(0,0,0);
    double r3 = run(pa, pa, pa, pa);
    ASSERT_EQ(r1, r2);
    ASSERT_EQ(r1, r3);
}
