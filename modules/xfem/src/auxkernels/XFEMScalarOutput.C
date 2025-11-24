//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "XFEMScalarOutput.h"
#include "petscblaslapack.h"
#include "XFEM.h"

registerMooseObject("XFEMApp", XFEMScalarOutput);

InputParameters
XFEMScalarOutput::validParams()
{
  InputParameters params = AuxKernel::validParams();
  params.addClassDescription(
    "Projects a scalar material property to the kept fragment of cut elements via a "
    "bilinear least-squares fit and reports its area-averaged elemental value.");
  params.addRequiredParam<std::string>(
      "property_name",
      "The name of the scalar material property to project");
  params.addParam<unsigned int>(
      "tri_quad_rule",
      2,
      "Triangle quadrature rule used for polygon integration on kept fragments");
  return params;
}

XFEMScalarOutput::XFEMScalarOutput(const InputParameters & parameters)
  : AuxKernel(parameters),
    _scalar_property(getMaterialProperty<Real>(parameters.get<std::string>("property_name")))
{
  if (isNodal())
    mooseError("XFEMScalarOutput must be run on an element variable");

  FEProblemBase * fe_problem = dynamic_cast<FEProblemBase *>(&_subproblem);
  if (fe_problem == nullptr)
    mooseError("Problem casting _subproblem to FEProblemBase in XFEMScalarOutput");

  _fe_problem = fe_problem;

  _xfem = MooseSharedNamespace::dynamic_pointer_cast<XFEM>(_fe_problem->getXFEM());
  if (_xfem == nullptr)
    mooseError("Problem casting to XFEM in XFEMScalarOutput");

  _tri_quad_rule = getParam<unsigned int>("tri_quad_rule");
}

Real
XFEMScalarOutput::computeValue()
{
  return _xfem->getPhysicalVolumeFraction(_current_elem);
}

void
XFEMScalarOutput::compute()
{
  const Elem * elem = _current_elem;

  // UNCUT ELEMENT
  if (!_xfem->isElemCut(elem))
  {
    Real int_val = 0.0;
    for (_qp = 0; _qp < _qrule->n_points(); ++_qp)
      int_val += _JxW[_qp] * _coord[_qp] * _scalar_property[_qp];

    const Real vol = (_bnd ? _current_side_volume : _current_elem_volume);
    const Real avg = (vol > 0.0) ? int_val / vol : 0.0;
    _var.setNodalValue(avg);
    return;
  }

  // CUT ELEMENT
  const std::vector<std::array<Point,3>> tris = buildPolygon(*_xfem, elem);

  // Recover polygon vertex order from the fan: {p_i} where tri = {c, p_i, p_{i+1}}
  std::vector<Point> poly_pts;
  poly_pts.reserve(tris.size());
  for (const auto & T : tris)
    poly_pts.push_back(T[1]);

  // Quadrature over the polygon via helper
  const unsigned int tri_rule = _tri_quad_rule ? _tri_quad_rule : 2;
  std::vector<Point> qpts;
  std::vector<Real>  qwts;
  getqRule(poly_pts, tri_rule, qpts, qwts);

  const unsigned int N = _qrule->n_points();

  // b (known values at existing element qps)
  std::vector<double> known_values(N, 0.0);
  for (unsigned int i = 0; i < N; ++i)
    known_values[i] = _scalar_property[i];

  // A: N x 4 with columns [1, x, y, x*y]
  std::vector<std::vector<double>> A(N, std::vector<double>(4, 0.0));
  for (unsigned int i = 0; i < N; ++i)
  {
    const double x = _q_point[i](0);
    const double y = _q_point[i](1);
    A[i][0] = 1.0;
    A[i][1] = x;
    A[i][2] = y;
    A[i][3] = x * y;
  }

  // Normal equations AtA * coeffs = Atb
  std::vector<std::vector<double>> AtA(4, std::vector<double>(4, 0.0));
  std::vector<double> Atb(4, 0.0);

  for (unsigned int i = 0; i < N; ++i)
  {
    for (int m = 0; m < 4; ++m)
    {
      Atb[m] += A[i][m] * known_values[i];
      for (int n = 0; n < 4; ++n)
        AtA[m][n] += A[i][m] * A[i][n];
    }
  }

  // Solve with LAPACK (column-major)
  std::vector<double> flatA(16, 0.0); // 4x4
  for (int col = 0; col < 4; ++col)
    for (int row = 0; row < 4; ++row)
      flatA[col * 4 + row] = AtA[row][col];

  std::vector<double> coeffs = Atb;
  std::vector<int> ipiv(4, 0);
  int n = 4, nrhs = 1, lda = 4, ldb = 4, info = 0;
  LAPACKgesv_(&n, &nrhs, flatA.data(), &lda, ipiv.data(), coeffs.data(), &ldb, &info);

  // Integrate LS-evaluated property over polygon quadrature
  const bool axisym =
      (_fe_problem->mesh().getCoordSystem(elem->subdomain_id()) == Moose::COORD_RZ);

  auto rz_measure = [](const Point & p) -> Real
  {
    const Real r = std::max<Real>(0.0, p(0));
    return 2.0 * libMesh::pi * r;
  };

  Real S = 0.0, Aarea = 0.0;
  for (std::size_t i = 0; i < qpts.size(); ++i)
  {
    Real wi = qwts[i];
    if (axisym)
      wi *= rz_measure(qpts[i]);

    const Real x = qpts[i](0);
    const Real y = qpts[i](1);
    const Real prop = coeffs[0] + coeffs[1]*x + coeffs[2]*y + coeffs[3]*x*y;

    S += prop * wi;
    Aarea += wi;
  }

  const Real avg = S / Aarea;
  _var.setNodalValue(avg);
}

static Real
polygonArea2D(const std::vector<Point> & poly)
{
  const std::size_t n = poly.size();
  if (n < 3)
    return 0.0;

  Real area = 0.0;
  for (std::size_t i = 0; i < n; ++i)
  {
    const std::size_t j = (i + 1) % n;
    const Real xi = poly[i](0);
    const Real yi = poly[i](1);
    const Real xj = poly[j](0);
    const Real yj = poly[j](1);
    area += xi * yj - xj * yi;
  }
  return 0.5 * area;
}

static void
orderPolygonCCW(std::vector<Point> & pts)
{
  if (pts.size() < 3)
    return;

  Point c(0.0, 0.0, 0.0);
  for (const auto & p : pts)
    c += p;
  c /= static_cast<Real>(pts.size());

  std::sort(pts.begin(),
            pts.end(),
            [&c](const Point & a, const Point & b)
            {
              const Real ax = a(0) - c(0);
              const Real ay = a(1) - c(1);
              const Real bx = b(0) - c(0);
              const Real by = b(1) - c(1);
              const Real anga = std::atan2(ay, ax);
              const Real angb = std::atan2(by, bx);
              return anga < angb;
            });
}

std::vector<std::array<Point, 3>>
buildPolygon(const XFEM & xfem, const Elem * elem)
{
  std::vector<std::array<Point, 3>> tris;

  // Get fragment faces
  std::vector<std::vector<Point>> frag_faces;
  xfem.getFragmentFaces(elem, frag_faces, /*displaced_mesh=*/false);

  if (frag_faces.empty())
  {
    mooseWarning("XFEMScalarOutput: getFragmentFaces returned no fragment faces for elem ",
                 elem->id());
    return tris;
  }

  int best_index = -1;
  Real best_abs_area = 0.0;
  for (unsigned int i = 0; i < frag_faces.size(); ++i)
  {
    const auto & poly = frag_faces[i];
    if (poly.size() < 3)
      continue;

    const Real a = std::abs(polygonArea2D(poly));
    if (a > best_abs_area)
    {
      best_abs_area = a;
      best_index = i;
    }
  }

  if (best_index >= 0)
  {
    const auto & poly = frag_faces[best_index];
    const Point c = xfem.getPhysicalCenterPoint(elem);
    for (std::size_t i = 0; i < poly.size(); ++i)
    {
      const Point & p1 = poly[i];
      const Point & p2 = poly[(i + 1) % poly.size()];
      tris.push_back({c, p1, p2});
    }
    return tris;
  }

  std::vector<Point> unique_pts;
  const Real tol2 = 1e-20;

  for (const auto & seg : frag_faces)
  {
    for (const auto & p : seg)
    {
      bool found = false;
      for (const auto & q : unique_pts)
      {
        const Real dx = p(0) - q(0);
        const Real dy = p(1) - q(1);
        const Real dz = p(2) - q(2);
        const Real dist2 = dx*dx + dy*dy + dz*dz;
        if (dist2 < tol2)
        {
          found = true;
          break;
        }
      }
      if (!found)
        unique_pts.push_back(p);
    }
  }

  if (unique_pts.size() < 3)
  {
    mooseWarning("XFEMScalarOutput: could not reconstruct polygon for elem ",
                 elem->id(),
                 " from ",
                 frag_faces.size(),
                 " fragment faces (unique points = ",
                 unique_pts.size(),
                 ").");
    return tris;
  }

  orderPolygonCCW(unique_pts);

  const Point c = xfem.getPhysicalCenterPoint(elem);
  for (std::size_t i = 0; i < unique_pts.size(); ++i)
  {
    const Point & p1 = unique_pts[i];
    const Point & p2 = unique_pts[(i + 1) % unique_pts.size()];
    tris.push_back({c, p1, p2});
  }

  return tris;
}

void
getqRule(const std::vector<Point> & poly_pts,
         unsigned int tri_rule,
         std::vector<Point> & quad_pts,
         std::vector<Real>  & quad_wts)
{
  quad_pts.clear();
  quad_wts.clear();

  const std::size_t n = poly_pts.size();
  if (n < 3)
    return;

  mooseAssert(tri_rule >= 1, "Triangle quadrature rule must be >= 1");

  Point xcrd(0.0, 0.0, 0.0);
  for (const auto & p : poly_pts)
    xcrd += p;
  xcrd /= static_cast<Real>(n);

  for (std::size_t j = 0; j < n; ++j)
  {
    const std::size_t j1 = (j + 1) % n;

    // Sub-triangle nodes: centroid, vertex j, vertex j+1
    std::vector<Point> subtri_pts(3);
    subtri_pts[0] = xcrd;
    subtri_pts[1] = poly_pts[j];
    subtri_pts[2] = poly_pts[j1];

    // Triangle quadrature on the reference triangle
    std::vector<std::vector<Real>> sg2;
    Xfem::stdQuadr2D(/*nnodes=*/3, /*rule=*/tri_rule, sg2);

    if (sg2.empty())
    {
      mooseWarning("XFEMScalarOutput: stdQuadr2D returned no points for tri_rule ",
                   tri_rule,
                   " in getqRule");
      continue;
    }

    std::vector<std::vector<Real>> shape(3, std::vector<Real>(3, 0.0));
    Real jac = 0.0;

    for (std::size_t l = 0; l < sg2.size(); ++l)
    {
      Xfem::shapeFunc2D(/*nnodes=*/3, sg2[l], subtri_pts, shape, jac, /*isPlanar=*/true);

      if (std::abs(jac) == 0.0)
      {
        mooseWarning("XFEMScalarOutput: zero jacobian in subtriangle quadrature in getqRule");
        continue;
      }

      Point xq(0.0, 0.0, 0.0);
      for (std::size_t k = 0; k < 3; ++k)
        xq += subtri_pts[k] * shape[k][2];  // N_k at the qp

      const Real wq = sg2[l][3] * std::abs(jac);

      quad_pts.push_back(xq);
      quad_wts.push_back(wq);
    }
  }
}