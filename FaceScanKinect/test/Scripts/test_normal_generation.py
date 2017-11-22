#! /usr/bin/env python

# Small test to verify the computation of
#   Covariance Matrix 
#   Eigenvalues and Eigenvectors  
#   Resulting normal

import numpy as np

# Example NearestNeighbor Data grabbed from Kinect Stream
points = np.array([
[0.0621294,  0.373613,  1.17315],
[0.0646813,  0.372088,  1.17511],
[0.0623281,  0.374629,  1.17664],
[0.0635704,  0.373529,  1.1765],
[0.0626942,  0.371844,  1.16987],
[0.0592679,  0.37622,   1.17396],
[0.0604626,  0.376027,  1.17625],
[0.0653953,  0.371962,  1.17666],
[0.0660504,  0.371242,  1.1763],
[0.0575083,  0.376042,  1.16983],
[0.057227,   0.377663,  1.17367],
[0.0586507,  0.377583,  1.17674],
[0.062509,   0.370833,  1.16721],
[0.0567664,  0.377007,  1.17089],
[0.0668918,  0.3689,    1.1721]
])

print 'Input Data'
print points

centroid =  np.sum(points, axis=0) / points.shape[0]
print 'Centroid'
print centroid


cov_myself = points - centroid
cov_myself = cov_myself.transpose().dot(cov_myself) / points.shape[0]
print 'Covariance manually computed'
print cov_myself


cov = np.cov(points, rowvar=False, bias=True)
print 'Covariance computed by numpy'
print cov

values, vectors = np.linalg.eig(cov)

print 'Eigenvalues und -vectors'
print values
print vectors

print 'Smallest Eigenvalue at Index'
print np.argmin(values)

print 'Corresponding Eigenvector'
print vectors[:,np.argmin(values)]
