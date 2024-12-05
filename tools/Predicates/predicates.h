/* This file is part of PyMesh. Copyright (c) 2019 by Qingnan Zhou */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#pragma GCC visibility push(hidden)
void prdc_exactinit();
double prdc_orient2d(double pa[2], double pb[2], double pc[2]);
double prdc_orient2dexact(double pa[2], double pb[2], double pc[2]);
double prdc_orient3d(double pa[3], double pb[3], double pc[3], double pd[3]);
double prdc_orient3dexact(double pa[3], double pb[3], double pc[3], double pd[3]);
double prdc_incircle(double pa[2], double pb[2], double pc[2], double pd[2]);
double prdc_incircleexact(double pa[2], double pb[2], double pc[2], double pd[2]);
double prdc_insphere(double pa[3], double pb[3], double pc[3], double pd[3], double pe[3]);
double prdc_insphereexact(double pa[3], double pb[3], double pc[3], double pd[3], double pe[3]);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif
