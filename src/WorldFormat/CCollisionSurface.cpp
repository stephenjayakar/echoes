#include "WorldFormat/CCollisionSurface.hpp"

CVector3f CCollisionSurface::GetNormal() const {
  const CVector3f a = mB - mA;
  const CVector3f b = mC - mA;
  return CVector3f::Cross(a, b).AsNormalized();
}

CPlane CCollisionSurface::GetPlane() const {
  const CUnitVector3f normal(GetNormal());
  return CPlane(CVector3f::Dot(normal, mA), normal);
}

// Guessed name
CPlane CCollisionSurface::GetEdgePlane(int edge) const {
  const CUnitVector3f normal(GetNormal());
  const int nextVertex[] = {1, 2, 0};
  const CVector3f& vertex = (&mA)[edge];
  const CVector3f edgeDirection = (&mA)[nextVertex[edge]] - vertex;
  const CUnitVector3f edgeNormal(CVector3f::Cross(normal, edgeDirection));
  return CPlane(CVector3f::Dot(edgeNormal, vertex), edgeNormal);
}

// Guessed name
bool CCollisionSurface::IsDegenerate() const {
  return mA == mB || mB == mC ||
         mA == mC;
}
