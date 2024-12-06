from .version import __version__
from . import PyMeshSetting
from .timethis import timethis

# Set default logging handler to avoid "No handler found" warnings.
import logging
try:  # Python 2.7+
    from logging import NullHandler
except ImportError:
    class NullHandler(logging.Handler):
        def emit(self, record):
            pass
logging.getLogger(__name__).addHandler(NullHandler())

from .Mesh import Mesh
from .meshio import load_mesh, form_mesh, save_mesh, save_mesh_raw
from .Assembler import Assembler
from .boolean import boolean
from .compression import compress, decompress
from .convex_hull import convex_hull
from .CSGTree import CSGTree
from .cut_to_disk import cut_to_disk
from .exact_arithmetic import Gmpz, Gmpq
from .HarmonicSolver import HarmonicSolver
from .HashGrid import HashGrid
from .material import Material
from .selfintersection import resolve_self_intersection
from .selfintersection import detect_self_intersection
from .outerhull import compute_outer_hull
from .winding_number import compute_winding_number
from .meshutils import *
from .misc import *
from .predicates import orient_3D, orient_2D, in_circle, in_sphere
from .slice_mesh import slice_mesh
from .submesh import submesh
from .VoxelGrid import VoxelGrid
from .tetrahedralize import tetrahedralize
from .matrixio import load_matrix, save_matrix
from .minkowski_sum import minkowski_sum
from .cell_partition import partition_into_cells
from .aabb_tree import AABBTree, AABBTree2, BVH, distance_to_mesh, signed_distance_to_mesh, do_intersect
from .triangle import triangle
from .triangulate import triangulate_beta, refine_triangulation
from .wires import *
from .snap_rounding import snap_rounding
from .SparseSolver import SparseSolver
from .straight_skeleton import straight_skeleton
from .igl_utils import unique_rows
from .igl_utils import face_normals, vertex_normals, edge_normals, orient_faces

from .map_attributes import map_vertex_attribute
from .map_attributes import map_face_attribute
from .map_attributes import map_corner_attribute
import nose2
import os


def test(verbose=3):
    to_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "tests")
    return nose2.main(argv=['nose2', '-s', to_dir, '--verbosity', str(verbose)], module=None)


__all__ = [
        "Mesh",
        "Assembler",
        "load_mesh",
        "form_mesh",
        "save_mesh",
        "save_mesh_raw",
        "boolean",
        "CSGTree",
        "cut_to_disk"
        "Gmpq",
        "Gmpz",
        "load_matrix",
        "Material",
        "save_matrix",
        "resolve_self_intersection",
        "detect_self_intersection",
        "compute_outer_hull",
        "compute_winding_number",
        "slice_mesh",
        "submesh",
        "timethis",
        "orient_3D",
        "orient_2D",
        "in_circle",
        "in_sphere",
        "slice_mesh",
        "triangle",
        "triangulate_beta",
        "tetrahedralize",
        "minkowski_sum",
        "partition_into_cells",
        "AABBTree",
        "AABBTree2",
        "distance_to_mesh",
        "signed_distance_to_mesh",
        "do_intersect",
        "VoxelGrid",
        "HarmonicSolver",
        "snap_rounding",
        "SparseSolver",
        "straight_skeleton",
        "HashGrid",
        "compress",
        "decompress",
        "map_vertex_attribute",
        "map_face_attribute",
        "map_corner_attribute",
        "refine_triangulation",
        "unique_rows",
        "face_normals",
        "vertex_normals",
        "edge_normals",
        "orient_faces",
        ]
__all__ += meshutils.__all__
__all__ += misc.__all__
__all__.append("wires")

try:
    from .Arrangement2 import Arrangement2
    __all__.append("Arrangement2")
except:
    logging.getLogger(__name__).warning(
            "Arrangement computation is unavailable")

try:
    from .tetgen import tetgen
    __all__.append("tetgen")
except:
    logging.getLogger(__name__).warning(
            "TetGen is unavailable")
