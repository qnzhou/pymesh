import numpy as np
from numpy.linalg import norm, svd
from math import pi,sin,cos,atan2,sqrt

class Quaternion:
    """ This class implements quaternion used for 3D rotations.

    Attributes:
        w (``float``): same as ``quaternion[0]``.
        x (``float``): same as ``quaternion[1]``.
        y (``float``): same as ``quaternion[2]``.
        z (``float``): same as ``quaternion[3]``.
    """

    def __init__(self, quat=None):
        if quat is None:
            quat = [1, 0, 0, 0]
        self.__quat = np.array(quat, dtype=np.float64)
        self.normalize()

    @classmethod
    def fromAxisAngle(cls, axis, angle):
        """ Crate quaternion from axis angle representation

        Args:
            angle (``float``): Angle in radian.
            axis (``numpy.ndarray``): Rotational axis.  Not necessarily
                normalized.

        Returns:
            The following values are returned.

            * ``quat`` (:class:`Quaternion`): The corresponding quaternion object.
        """
        axis = axis / norm(axis)
        quat = np.array([
            cos(angle/2),
            sin(angle/2)*axis[0],
            sin(angle/2)*axis[1],
            sin(angle/2)*axis[2]
            ])
        return cls(quat)

    @classmethod
    def fromData(cls, v1, v2):
        """ Create the rotation to rotate v1 to v2

        Args:
            v1 (``numpy.ndarray``): From vector. Normalization not necessary.
            v2 (``numpy.ndarray``): To vector. Normalization not necessary.

        Returns:
            The following values are returned.

            * ``quat`` (:class:`Quaternion`): Corresponding quaternion that
              rotates ``v1`` to ``v2``.
        """
        eps = 1e-12
        v1 /= norm(v1)
        v2 /= norm(v2)
        c = np.dot(v1, v2)
        if c < -1.0 + eps:
            # v1 is parallel and opposite of v2
            u,s,v = svd(np.array([v1, v2]))
            axis = v[2,:]
        else:
            axis = np.cross(v1, v2)
            l = norm(axis)
            if l > 0.0:
                axis /= norm(axis)
            else:
                # Parallel vectors.
                axis = v1

        w_sq = 0.5 * (1.0 + c)
        l = sqrt(1.0 - w_sq) * axis
        quat = np.array([sqrt(w_sq), l[0], l[1], l[2]])
        return cls(quat)

    def norm(self):
        """ Quaternion norm.
        """
        n = norm(self.__quat)
        return n

    def normalize(self):
        """ Normalize quaterion to have length 1.
        """
        n = self.norm()
        if n == 0:
            print(self.__quat)
            raise ZeroDivisionError("quaternion cannot be 0!")

        self.__quat /= n

    def __str__(self):
        return str(self.__quat)

    def __getitem__(self, i):
        return self.__quat[i]

    def __setitem__(self, i, val):
        self.__quat[i] = val

    def __mul__(self, other):
        """ Multiplication: ``self`` * ``other``

        Args:
            ``other`` (:class:`Quaternion` or ``numpy.ndarray``): If it is of
                type ``numpy.ndarray``, it must be normalized.
        """
        r = Quaternion()
        a = self
        b = other
        r[0] = a[0]*b[0] - a[1]*b[1] - a[2]*b[2] - a[3]*b[3]
        r[1] = a[0]*b[1] + a[1]*b[0] + a[2]*b[3] - a[3]*b[2]
        r[2] = a[0]*b[2] - a[1]*b[3] + a[2]*b[0] + a[3]*b[1]
        r[3] = a[0]*b[3] + a[1]*b[2] - a[2]*b[1] + a[3]*b[0]
        return r

    def __rmul__(self, other):
        """ Multiplication: ``other`` * ``self``

        This method is called only if other is not a :class:`Quaternion`.

        Args:
            ``other`` (:class:`Quaternion` or ``numpy.ndarray``): If it is of
                type ``numpy.ndarray``, it must be normalized.
        """
        r = Quaternion()
        a = other
        b = self
        r[0] = a[0]*b[0] - a[1]*b[1] - a[2]*b[2] - a[3]*b[3]
        r[1] = a[0]*b[1] + a[1]*b[0] + a[2]*b[3] - a[3]*b[2]
        r[2] = a[0]*b[2] - a[1]*b[3] + a[2]*b[0] + a[3]*b[1]
        r[3] = a[0]*b[3] + a[1]*b[2] - a[2]*b[1] + a[3]*b[0]
        return r

    def to_matrix(self):
        """ Convert to rotational matrix.

        Returns:
            ``numpy.ndarray``: The corresponding rotational matrix.
        """
        a = self.__quat
        return np.array([
            [1 - 2*a[2]*a[2] -2*a[3]*a[3], 2*a[1]*a[2] - 2*a[3]*a[0], 2*a[1]*a[3] + 2*a[2]*a[0]],
            [2*a[1]*a[2] + 2*a[3]*a[0], 1 - 2*a[1]*a[1] -2*a[3]*a[3], 2*a[2]*a[3] - 2*a[1]*a[0]],
            [2*a[1]*a[3] - 2*a[2]*a[0], 2*a[2]*a[3] + 2*a[1]*a[0], 1 - 2*a[1]*a[1] -2*a[2]*a[2]],
            ])

    def conjugate(self):
        """ returns the conjugate of this quaternion, does nothing to self.
        """
        return Quaternion([
            self.__quat[0],
            -1 * self.__quat[1],
            -1 * self.__quat[2],
            -1 * self.__quat[3]
            ])

    def rotate(self, v):
        """ Rotate 3D vector v by this quaternion

        Args:
            ``v`` (``numpy.ndarray``): Must be 1D vector.

        Returns:
            The rotated vector.
        """
        m = norm(v)
        v = Quaternion([0, v[0], v[1], v[2]])
        r = self * v * self.conjugate()
        return r[1:4] * m

    @property
    def w(self):
        return self.__quat[0]

    @property
    def x(self):
        return self.__quat[1]

    @property
    def y(self):
        return self.__quat[2]

    @property
    def z(self):
        return self.__quat[3]

