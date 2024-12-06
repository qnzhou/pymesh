#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

extern "C"
{
#include <Predicates/predicates.h>
}

namespace py = pybind11;
using Arr2D = Eigen::Matrix<double, 2, 1>;
using Arr3D = Eigen::Matrix<double, 3, 1>;

void init_predicates(py::module& m) {
    m.def("exactinit", &prdc_exactinit);
    m.def("orient2d",
            [](Arr2D& pa, Arr2D& pb, Arr2D& pc) {
            return prdc_orient2d(pa.data(), pb.data(), pc.data());
            });
    m.def("orient2dexact",
            [](Arr2D& pa, Arr2D& pb, Arr2D& pc) {
            return prdc_orient2dexact(pa.data(), pb.data(), pc.data());
            });
    m.def("orient3d",
            [](Arr3D& pa, Arr3D& pb, Arr3D& pc, Arr3D& pd) {
            return prdc_orient3d(pa.data(), pb.data(), pc.data(), pd.data());
            });
    m.def("orient3dexact",
            [](Arr3D& pa, Arr3D& pb, Arr3D& pc, Arr3D& pd) {
            return prdc_orient3dexact(pa.data(), pb.data(), pc.data(), pd.data());
            });
    m.def("incircle",
            [](Arr2D& pa, Arr2D& pb, Arr2D& pc, Arr2D& pd) {
            return prdc_incircle(pa.data(), pb.data(), pc.data(), pd.data());
            });
    m.def("incircleexact",
            [](Arr2D& pa, Arr2D& pb, Arr2D& pc, Arr2D& pd) {
            return prdc_incircleexact(pa.data(), pb.data(), pc.data(), pd.data());
            });
    m.def("insphere",
            [](Arr3D& pa, Arr3D& pb, Arr3D& pc, Arr3D& pd, Arr3D& pe) {
            return prdc_insphere(pa.data(), pb.data(), pc.data(), pd.data(), pe.data());
            });
    m.def("insphereexact",
            [](Arr3D& pa, Arr3D& pb, Arr3D& pc, Arr3D& pd, Arr3D& pe) {
            return prdc_insphereexact(pa.data(), pb.data(), pc.data(), pd.data(), pe.data());
            });
}
