// $Id$
//==============================================================================
//!
//! \file ASMs2DLag.C
//!
//! \date Mar 20 2010
//!
//! \author Einar Christensen / SINTEF
//!
//! \brief Driver for assembly of structured 2D Lagrange FE models.
//!
//==============================================================================

#include "GoTools/geometry/SplineSurface.h"

#include "ASMs2DLag.h"
#include "Lagrange.h"
#include "TimeDomain.h"
#include "FiniteElement.h"
#include "GlobalIntegral.h"
#include "LocalIntegral.h"
#include "IntegrandBase.h"
#include "CoordinateMapping.h"
#include "GaussQuadrature.h"
#include "ElementBlock.h"
#include "Utilities.h"
#include "Vec3Oper.h"
#include <array>


ASMs2DLag::ASMs2DLag (unsigned char n_s, unsigned char n_f)
  : ASMs2D(n_s,n_f), coord(myCoord)
{
  nx = ny = 0;
  p1 = p2 = 0;
}


ASMs2DLag::ASMs2DLag (const ASMs2DLag& patch, unsigned char n_f)
  : ASMs2D(patch,n_f), coord(patch.myCoord)
{
  nx = patch.nx;
  ny = patch.ny;
  p1 = patch.p1;
  p2 = patch.p2;
}


ASMs2DLag::ASMs2DLag (const ASMs2DLag& patch)
  : ASMs2D(patch), coord(myCoord), myCoord(patch.coord)
{
  nx = patch.nx;
  ny = patch.ny;
  p1 = patch.p1;
  p2 = patch.p2;
}


void ASMs2DLag::clear (bool retainGeometry)
{
  myCoord.clear();
  nx = ny = 0;
  p1 = p2 = 0;

  this->ASMs2D::clear(retainGeometry);
}


bool ASMs2DLag::addXElms (short int dim, short int item, size_t nXn,
                          IntVec& nodes)
{
  if (!this->addXNodes(dim,nXn,nodes))
    return false;
  else if (p1 < 2 || p2 < 2)
    return false;

  const int nelx = (nx-1)/(p1-1);
  const int nely = (ny-1)/(p2-1);

  int iel = 0;
  bool skipMe = false;
  for (int i2 = 0; i2 < nely; i2++)
    for (int i1 = 0; i1 < nelx; i1++, iel++)
    {
      if (MLGE[iel] < 1) continue; // Skip zero-area element

      // Skip elements that are not on current boundary edge
      switch (item)
        {
        case 1: skipMe = i1 > 0;      break;
        case 2: skipMe = i1 < nelx-1; break;
        case 3: skipMe = i2 > 0;      break;
        case 4: skipMe = i2 < nely-1; break;
        }
      if (skipMe) continue;

      IntVec& mnpc = myMNPC[nel+iel];
      if (!mnpc.empty())
      {
        std::cerr <<" *** ASMs2DLag::addXElms: Only one X-edge allowed."
                  << std::endl;
        return false;
      }

      mnpc = MNPC[iel]; // Copy the ordinary element nodes

      // Negate node numbers that are not on the boundary edge, to flag that
      // they shall not receive any tangent and/or residual contributions
      int lnod = 0;
      for (int j2 = 0; j2 < p2; j2++)
        for (int j1 = 0; j1 < p1; j1++, lnod++)
        {
          switch (item)
            {
            case 1: skipMe = j1 > 0;    break;
            case 2: skipMe = j1 < p1-1; break;
            case 3: skipMe = j2 > 0;    break;
            case 4: skipMe = j2 < p2-1; break;
	    }
	  if (skipMe) // Hack for node 0: Using -maxint as flag instead
	    mnpc[lnod] = mnpc[lnod] == 0 ? -2147483648 : -mnpc[lnod];
	}

      // Add connectivity to the extra-ordinary nodes
      for (size_t i = 0; i < nXn; i++)
        mnpc.push_back(MLGN.size()-nXn+i);

      myMLGE[nel+iel] = -(++gEl); // Flag extraordinary element by negative sign
    }

  return true;
}


bool ASMs2DLag::generateFEMTopology ()
{
  if (!surf) return false;
  if (!proj) proj = surf;

  // Order of basis in the two parametric directions (order = degree + 1)
  p1 = surf->order_u();
  p2 = surf->order_v();

  // Evaluate the parametric values
  RealArray gpar1, gpar2;
  if (!this->getGridParameters(gpar1,0,p1-1)) return false;
  if (!this->getGridParameters(gpar2,1,p2-1)) return false;

  // Number of nodes in each direction
  nx = gpar1.size();
  ny = gpar2.size();
  // Number of nodes in the patch
  nnod = nx*ny;

  if (!coord.empty())
    return coord.size() == nnod;

  // Number of elements in each direction
  const int nelx = (nx-1)/(p1-1);
  const int nely = (ny-1)/(p2-1);

  // Evaluate the nodal coordinates in the physical space
  RealArray XYZ(surf->dimension()*nnod);
  surf->gridEvaluator(XYZ,gpar1,gpar2);

  size_t i1, j1;
  myMLGN.resize(nnod);
  myCoord.resize(nnod);
  for (i1 = j1 = 0; i1 < myCoord.size(); i1++)
  {
    myMLGN[i1] = ++gNod;
    for (size_t d = 0; d < nsd; d++)
      myCoord[i1][d] = XYZ[j1+d];
    j1 += surf->dimension();
  }

  // Number of elements in patch
  nel = nelx*nely;
  // Number of nodes per element
  const int nen = p1*p2;

  // Connectivity array: local --> global node relation
  myMLGE.resize(nel);
  myMNPC.resize(nel);

  int i, j, a, b, iel = 0;
  for (j = 0; j < nely; j++)
    for (i = 0; i < nelx; i++, iel++)
    {
      myMLGE[iel] = ++gEl;
      myMNPC[iel].resize(nen);
      // First node in current element
      int corner = (p2-1)*nx*j + (p1-1)*i;

      for (b = 0; b < p2; b++)
      {
	int facenod = b*p1;
	myMNPC[iel][facenod] = corner + b*nx;
	for (a = 1; a < p1; a++)
	  myMNPC[iel][facenod+a] = myMNPC[iel][facenod] + a;
      }
    }

  return true;
}


Vec3 ASMs2DLag::getCoord (size_t inod) const
{
  if (inod < 1 || inod > coord.size()) return Vec3();

  return coord[inod-1];
}


void ASMs2DLag::setCoord (size_t inod, const Vec3& Xnod)
{
  if (inod > nnod)
    myCoord.resize(nnod = inod);

  myCoord[inod-1] = Xnod;
}


bool ASMs2DLag::getElementCoordinates (Matrix& X, int iel) const
{
  if (iel < 1 || (size_t)iel > MNPC.size())
  {
    std::cerr <<" *** ASMs2DLag::getElementCoordinates: Element index "<< iel
	      <<" out of range [1,"<< MNPC.size() <<"]."<< std::endl;
    return false;
  }

  // Number of nodes per element
  size_t nen = p1*p2;
  if (nen > MNPC[--iel].size()) nen = MNPC[iel].size();

  X.resize(nsd,nen);
  for (size_t i = 0; i < nen; i++)
    X.fillColumn(i+1,coord[MNPC[iel][i]].ptr());

  return true;
}


void ASMs2DLag::getNodalCoordinates (Matrix& X) const
{
  X.resize(nsd,coord.size());

  for (size_t inod = 0; inod < coord.size(); inod++)
    X.fillColumn(inod+1,coord[inod].ptr());
}


bool ASMs2DLag::updateCoords (const Vector& displ)
{
  if (shareFE) return true;

  if (displ.size() != nsd*coord.size())
  {
    std::cerr <<" *** ASMs2DLag::updateCoords: Invalid dimension "
	      << displ.size() <<" on displ, should be "
	      << nsd*coord.size() << std::endl;
    return false;
  }

  const double* u = displ.ptr();
  for (size_t inod = 0; inod < coord.size(); inod++, u += nsd)
    myCoord[inod] += RealArray(u,u+nsd);

  return true;
}


bool ASMs2DLag::getSize (int& n1, int& n2, int) const
{
  n1 = nx;
  n2 = ny;

  return true;
}


size_t ASMs2DLag::getNoBoundaryElms (char lIndex, char ldim) const
{
  if (ldim < 1 && lIndex > 0)
    return 1;

  switch (lIndex)
    {
    case 1:
    case 2:
      return p2 > 1 ? (ny-1)/(p2-1) : 0;
    case 3:
    case 4:
      return p1 > 1 ? (nx-1)/(p1-1) : 0;
    }

  return 0;
}


bool ASMs2DLag::integrate (Integrand& integrand,
			   GlobalIntegral& glInt,
			   const TimeDomain& time)
{
  if (this->empty()) return true; // silently ignore empty patches

  // Get Gaussian quadrature points and weights
  std::array<int,2> ng;
  std::array<const double*,2> xg, wg;
  for (int d = 0; d < 2; d++)
  {
    ng[d] = this->getNoGaussPt(d == 0 ? p1 : p2);
    xg[d] = GaussQuadrature::getCoord(ng[d]);
    wg[d] = GaussQuadrature::getWeight(ng[d]);
    if (!xg[d] || !wg[d]) return false;
  }

  // Get the reduced integration quadrature points, if needed
  const double* xr = nullptr;
  const double* wr = nullptr;
  int nRed = integrand.getReducedIntegration(ng[0]);
  if (nRed > 0)
  {
    xr = GaussQuadrature::getCoord(nRed);
    wr = GaussQuadrature::getWeight(nRed);
    if (!xr || !wr) return false;
  }
  else if (nRed < 0)
    nRed = ng[0]; // The integrand needs to know nGauss

  // Get parametric coordinates of the elements
  RealArray upar, vpar;
  this->getGridParameters(upar,0,1);
  this->getGridParameters(vpar,1,1);

  // Number of elements in each direction
  const int nelx = upar.empty() ? 0 : upar.size() - 1;


  // === Assembly loop over all elements in the patch ==========================

  bool ok = true;
  for (size_t g = 0; g < threadGroups.size() && ok; g++)
  {
#pragma omp parallel for schedule(static)
    for (size_t t = 0; t < threadGroups[g].size(); t++)
    {
      FiniteElement fe(p1*p2);
      Matrix dNdu, Xnod, Jac;
      Vec4   X;
      for (size_t i = 0; i < threadGroups[g][t].size() && ok; i++)
      {
        int iel = threadGroups[g][t][i];
        int i1  = nelx > 0 ? iel % nelx : 0;
        int i2  = nelx > 0 ? iel / nelx : 0;

        // Set up nodal point coordinates for current element
        if (!this->getElementCoordinates(Xnod,1+iel))
        {
          ok = false;
          break;
        }

        if (integrand.getIntegrandType() & Integrand::ELEMENT_CENTER)
        {
          // Compute the element "center" (average of element node coordinates)
          X = 0.0;
          for (size_t i = 1; i <= nsd; i++)
            for (size_t j = 1; j <= Xnod.cols(); j++)
              X[i-1] += Xnod(i,j);

          X *= 1.0/(double)Xnod.cols();
        }

        // Initialize element quantities
        fe.iel = MLGE[iel];
        LocalIntegral* A = integrand.getLocalIntegral(fe.N.size(),fe.iel);
        if (!integrand.initElement(MNPC[iel],fe,X,nRed*nRed,*A))
        {
          A->destruct();
          ok = false;
          break;
        }

        if (xr)
        {
          // --- Selective reduced integration loop ----------------------------

          for (int j = 0; j < nRed; j++)
            for (int i = 0; i < nRed; i++)
            {
              // Local element coordinates of current integration point
              fe.xi  = xr[i];
              fe.eta = xr[j];

              // Parameter value of current integration point
              if (!upar.empty())
                fe.u = 0.5*(upar[i1]*(1.0-xr[i]) + upar[i1+1]*(1.0+xr[i]));
              if (!vpar.empty())
                fe.v = 0.5*(vpar[i2]*(1.0-xr[j]) + vpar[i2+1]*(1.0+xr[j]));

              // Compute basis function derivatives at current point
              // using tensor product of one-dimensional Lagrange polynomials
              if (!Lagrange::computeBasis(fe.N,dNdu,p1,xr[i],p2,xr[j]))
                ok = false;

              // Compute Jacobian inverse and derivatives
              fe.detJxW = utl::Jacobian(Jac,fe.dNdX,Xnod,dNdu);

              // Cartesian coordinates of current integration point
              X = Xnod * fe.N;
              X.t = time.t;

              // Compute the reduced integration terms of the integrand
              fe.detJxW *= wr[i]*wr[j];
              if (!integrand.reducedInt(*A,fe,X))
                ok = false;
            }
        }


        // --- Integration loop over all Gauss points in each direction --------

        int jp = iel*ng[0]*ng[1];
        fe.iGP = firstIp + jp; // Global integration point counter

        for (int j = 0; j < ng[1]; j++)
          for (int i = 0; i < ng[0]; i++, fe.iGP++)
          {
            // Local element coordinates of current integration point
            fe.xi  = xg[0][i];
            fe.eta = xg[1][j];

            // Parameter value of current integration point
            if (!upar.empty())
              fe.u = 0.5*(upar[i1]*(1.0-xg[0][i]) + upar[i1+1]*(1.0+xg[0][i]));
            if (!vpar.empty())
              fe.v = 0.5*(vpar[i2]*(1.0-xg[1][j]) + vpar[i2+1]*(1.0+xg[1][j]));

            // Compute basis function derivatives at current integration point
            // using tensor product of one-dimensional Lagrange polynomials
            if (!Lagrange::computeBasis(fe.N,dNdu,p1,xg[0][i],p2,xg[1][j]))
              ok = false;

            // Compute Jacobian inverse of coordinate mapping and derivatives
            fe.detJxW = utl::Jacobian(Jac,fe.dNdX,Xnod,dNdu);
            if (fe.detJxW == 0.0) continue; // skip singular points

            // Cartesian coordinates of current integration point
            X = Xnod * fe.N;
            X.t = time.t;

            // Evaluate the integrand and accumulate element contributions
            fe.detJxW *= wg[0][i]*wg[1][j];
            if (!integrand.evalInt(*A,fe,time,X))
              ok = false;
          }

        // Finalize the element quantities
        if (ok && !integrand.finalizeElement(*A,time,firstIp+jp))
          ok = false;

        // Assembly of global system integral
        if (ok && !glInt.assemble(A->ref(),fe.iel))
          ok = false;

        A->destruct();
      }
    }
  }

  return ok;
}


bool ASMs2DLag::integrate (Integrand& integrand, int lIndex,
			   GlobalIntegral& glInt,
			   const TimeDomain& time)
{
  if (this->empty()) return true; // silently ignore empty patches

  // Get Gaussian quadrature points and weights
  int nG1 = this->getNoGaussPt(lIndex%10 < 3 ? p2 : p1, true);
  int nGP = integrand.getBouIntegrationPoints(nG1);
  const double* xg = GaussQuadrature::getCoord(nGP);
  const double* wg = GaussQuadrature::getWeight(nGP);
  if (!xg || !wg) return false;

  // Find the parametric direction of the edge normal {-2,-1, 1, 2}
  const int edgeDir = (lIndex%10+1)/((lIndex%2) ? -2 : 2);

  const int t1 = abs(edgeDir); // tangent direction normal to the patch edge
  const int t2 = 3-t1;         // tangent direction along the patch edge

  // Number of elements in each direction
  const int nelx = (nx-1)/(p1-1);
  const int nely = (ny-1)/(p2-1);

  // Get parametric coordinates of the elements
  FiniteElement fe(p1*p2);
  RealArray upar, vpar;
  if (t1 == 1)
  {
    fe.u = edgeDir < 0 ? surf->startparam_u() : surf->endparam_u();
    this->getGridParameters(vpar,1,1);
  }
  else if (t1 == 2)
  {
    this->getGridParameters(upar,0,1);
    fe.v = edgeDir < 0 ? surf->startparam_v() : surf->endparam_v();
  }

  // Extract the Neumann order flag (1 or higher) for the integrand
  integrand.setNeumannOrder(1 + lIndex/10);

  // Integrate the extraordinary elements?
  size_t doXelms = 0;
  if (integrand.getIntegrandType() & Integrand::XO_ELEMENTS)
    if ((doXelms = nelx*nely)*2 > MNPC.size())
    {
      std::cerr <<" *** ASMs2DLag::integrate: Too few XO-elements "
                << MNPC.size() - doXelms << std::endl;
      return false;
    }

  std::map<char,size_t>::const_iterator iit = firstBp.find(lIndex%10);
  size_t firstp = iit == firstBp.end() ? 0 : iit->second;

  Matrix dNdu, Xnod, Jac;
  Vec4   X;
  Vec3   normal;
  double xi[2];


  // === Assembly loop over all elements on the patch edge =====================

  int iel = 1;
  for (int i2 = 0; i2 < nely; i2++)
    for (int i1 = 0; i1 < nelx; i1++, iel++)
    {
      // Skip elements that are not on current boundary edge
      bool skipMe = false;
      switch (edgeDir)
	{
	case -1: if (i1 > 0)      skipMe = true; break;
	case  1: if (i1 < nelx-1) skipMe = true; break;
	case -2: if (i2 > 0)      skipMe = true; break;
	case  2: if (i2 < nely-1) skipMe = true; break;
	}
      if (skipMe) continue;

      // Set up nodal point coordinates for current element
      if (!this->getElementCoordinates(Xnod,iel)) return false;

      // Initialize element quantities
      fe.iel = abs(MLGE[doXelms+iel-1]);
      LocalIntegral* A = integrand.getLocalIntegral(fe.N.size(),fe.iel,true);
      bool ok = integrand.initElementBou(MNPC[doXelms+iel-1],*A);


      // --- Integration loop over all Gauss points along the edge -------------

      int jp = (t1 == 1 ? i2 : i1)*nGP;
      fe.iGP = firstp + jp; // Global integration point counter

      for (int i = 0; i < nGP && ok; i++, fe.iGP++)
      {
	// Local element coordinates of current integration point
	xi[t1-1] = edgeDir < 0 ? -1.0 : 1.0;
	xi[t2-1] = xg[i];
	fe.xi  = xi[0];
	fe.eta = xi[1];

	// Parameter values of current integration point
	if (upar.size() > 1)
	  fe.u = 0.5*(upar[i1]*(1.0-xg[i]) + upar[i1+1]*(1.0+xg[i]));
	if (vpar.size() > 1)
	  fe.v = 0.5*(vpar[i2]*(1.0-xg[i]) + vpar[i2+1]*(1.0+xg[i]));

	// Compute the basis functions and their derivatives, using
	// tensor product of one-dimensional Lagrange polynomials
	if (!Lagrange::computeBasis(fe.N,dNdu,p1,xi[0],p2,xi[1]))
	  ok = false;

	// Compute basis function derivatives and the edge normal
	fe.detJxW = utl::Jacobian(Jac,normal,fe.dNdX,Xnod,dNdu,t1,t2);
	if (fe.detJxW == 0.0) continue; // skip singular points

	if (edgeDir < 0) normal *= -1.0;

	// Cartesian coordinates of current integration point
	X = Xnod * fe.N;
	X.t = time.t;

	// Evaluate the integrand and accumulate element contributions
	fe.detJxW *= wg[i];
        if (ok && !integrand.evalBou(*A,fe,time,X,normal))
	  ok = false;
      }

      // Finalize the element quantities
      if (ok && !integrand.finalizeElementBou(*A,fe,time))
        ok = false;

      // Assembly of global system integral
      if (ok && !glInt.assemble(A->ref(),fe.iel))
	ok = false;

      A->destruct();

      if (!ok) return false;
    }

  return true;
}


int ASMs2DLag::evalPoint (const double* xi, double* param, Vec3& X) const
{
  if (surf)
  {
    param[0] = (1.0-xi[0])*surf->startparam_u() + xi[0]*surf->endparam_u();
    param[1] = (1.0-xi[1])*surf->startparam_v() + xi[1]*surf->endparam_v();
  }
  else
    memcpy(param,xi,2*sizeof(double));

  // Evaluate the parametric values of the nodes
  RealArray u, v;
  if (!this->getGridParameters(u,0,p1-1)) return -2;
  if (!this->getGridParameters(v,1,p2-1)) return -2;

  // Search for the closest node
  size_t i = utl::find_closest(u,param[0]);
  size_t j = utl::find_closest(v,param[1]);
  size_t n = u.size()*j + i;
  X = coord[n];

  return 1+n;
}


bool ASMs2DLag::tesselate (ElementBlock& grid, const int* npe) const
{
  if (p1 != npe[0] || p2 != npe[1])
  {
    int* newnpe = const_cast<int*>(npe);
    std::cout <<"\nLagrange elements: The number of visualization points are "
	      << p1 <<" "<< p2 <<" by default\n"<< std::endl;
    newnpe[0] = p1;
    newnpe[1] = p2;
  }

  if (!this->ASMs2D::tesselate(grid,npe))
    return false;

  // Adjust the element Id since each Lagrange element covers several knot-spans
  int i, ie, nse1 = p1-1;
  int j, je, nse2 = p2-1;
  int nelx = (nx-1)/nse1;
  for (j = je = 1; j < (int)ny; j++)
  {
    for (i = ie = 1; i < (int)nx; i++)
    {
      grid.setElmId((j-1)*(nx-1)+i,(je-1)*nelx+ie);
      if (i%nse1 == 0) ie++;
    }
    if (j%nse2 == 0) je++;
  }

  return true;
}


bool ASMs2DLag::evalSolution (Matrix& sField, const Vector& locSol,
                              const int*, int nf) const
{
  return this->evalSolution(sField,locSol,nullptr,true,0,nf);
}


bool ASMs2DLag::evalSolution (Matrix& sField, const Vector& locSol,
                              const RealArray*, bool, int, int) const
{
  size_t nPoints = coord.size();
  size_t nNodes = this->getNoNodes(-1);
  size_t nComp = locSol.size() / nNodes;
  if (nNodes < nPoints || nComp*nNodes != locSol.size())
    return false;

  sField.resize(nComp,nPoints);
  const double* u = locSol.ptr();
  for (size_t n = 1; n <= nPoints; n++, u += nComp)
    sField.fillColumn(n,u);

  return true;
}


bool ASMs2DLag::evalSolution (Matrix& sField, const IntegrandBase& integrand,
			      const int*, char) const
{
  return this->evalSolution(sField,integrand,(const RealArray*)nullptr);
}


bool ASMs2DLag::evalSolution (Matrix& sField, const IntegrandBase& integrand,
			      const RealArray*, bool) const
{
  sField.resize(0,0);

  double incx = 2.0/double(p1-1);
  double incy = 2.0/double(p2-1);

  size_t nPoints = coord.size();
  IntVec check(nPoints,0);

  FiniteElement fe(p1*p2);
  Vector        solPt;
  Vectors       globSolPt(nPoints);
  Matrix        dNdu, Xnod, Jac;

  // Evaluate the secondary solution field at each point
  const int nel = this->getNoElms(true);
  for (int iel = 1; iel <= nel; iel++)
  {
    const IntVec& mnpc = MNPC[iel-1];
    this->getElementCoordinates(Xnod,iel);

    int i, j, loc = 0;
    for (j = 0; j < p2; j++)
      for (i = 0; i < p1; i++, loc++)
      {
	fe.xi  = -1.0 + i*incx;
	fe.eta = -1.0 + j*incy;
	if (!Lagrange::computeBasis(fe.N,dNdu,p1,fe.xi,p2,fe.eta))
	  return false;

	fe.iel = MLGE[iel-1];

	// Compute the Jacobian inverse
	fe.detJxW = utl::Jacobian(Jac,fe.dNdX,Xnod,dNdu);

	// Now evaluate the solution field
	if (!integrand.evalSol(solPt,fe,Xnod*fe.N,mnpc))
	  return false;
	else if (sField.empty())
	  sField.resize(solPt.size(),nPoints,true);

	if (++check[mnpc[loc]] == 1)
	  globSolPt[mnpc[loc]] = solPt;
	else
	  globSolPt[mnpc[loc]] += solPt;
      }
  }

  for (size_t i = 0; i < nPoints; i++)
    sField.fillColumn(1+i,globSolPt[i] /= check[i]);

  return true;
}


void ASMs2DLag::generateThreadGroups (const Integrand&, bool, bool)
{
  threadGroups.calcGroups((nx-1)/(p1-1),(ny-1)/(p2-1),1);
}


bool ASMs2DLag::write(std::ostream& os, int) const
{
  return this->writeLagBasis(os, "quad");
}
