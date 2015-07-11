#
#  Copyright (c) 2009-2015, Jack Poulson
#  All rights reserved.
#
#  This file is part of Elemental and is under the BSD 2-Clause License, 
#  which can be found in the LICENSE file in the root directory, or at 
#  http://opensource.org/licenses/BSD-2-Clause
#
import El

m = 6000
n = 4000
rho = 10

display = True
worldRank = El.mpi.WorldRank()
worldSize = El.mpi.WorldSize()

# Make a sparse matrix with the last column dense
def Rectang(height,width):
  A = El.DistSparseMatrix()
  A.Resize(height,width)
  localHeight = A.LocalHeight()
  A.Reserve(5*localHeight)
  for sLoc in xrange(localHeight):
    s = A.GlobalRow(sLoc)
    if s < width: 
      A.QueueLocalUpdate( sLoc, s,        11 )
    if s >= 1 and s-1 < width:
      A.QueueLocalUpdate( sLoc, s-1,      -1 )
    if s+1 < width:
      A.QueueLocalUpdate( sLoc, s+1,       2 )
    if s >= height and s-height < width:
      A.QueueLocalUpdate( sLoc, s-height, -3 )
    if s+height < width: 
      A.QueueLocalUpdate( sLoc, s+height,  4 )
    # The dense last column
    A.QueueLocalUpdate( sLoc, width-1, -5/height );

  A.ProcessQueues()
  return A

A = Rectang(m,n)
b = El.DistMultiVec()
El.Gaussian( b, m, 1 )
if display:
  El.Display( A, "A" )
  El.Display( b, "b" )

ctrl = El.SOCPAffineCtrl_d()
ctrl.mehrotraCtrl.progress = True
ctrl.mehrotraCtrl.time = True
ctrl.mehrotraCtrl.qsdCtrl.progress = True
startRLS = El.mpi.Time()
x = El.RLS( A, b, rho, ctrl )
endRLS = El.mpi.Time()
if worldRank == 0:
  print "RLS time:", endRLS-startRLS, "seconds"
if display:
  El.Display( x, "x" )

e = El.DistMultiVec()
El.Copy( b, e )
El.Multiply( El.NORMAL, -1., A, x, 1., e )
eTwoNorm = El.Nrm2( e )
if worldRank == 0:
  print "|| A x - b ||_2 =", eTwoNorm

startLS = El.mpi.Time()
xLS = El.LeastSquares( A, b )
endLS = El.mpi.Time()
if worldRank == 0:
  print "LS time:", endLS-startLS, "seconds"
if display:
  El.Display( xLS, "xLS" )
El.Copy( b, e )
El.Multiply( El.NORMAL, -1., A, xLS, 1., e )
eTwoNorm = El.Nrm2( e )
if worldRank == 0:
  print "|| A x_{LS} - b ||_2 =", eTwoNorm

# Require the user to press a button before the figures are closed
El.Finalize()
if worldSize == 1:
  raw_input('Press Enter to exit')
