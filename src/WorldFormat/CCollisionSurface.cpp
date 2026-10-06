#include "WorldFormat/CCollisionSurface.hpp"

CVector3f CCollisionSurface::GetNormal() const {
  const CVector3f a = mVertices[1] - mVertices[0];
  const CVector3f b = mVertices[2] - mVertices[0];
  return CVector3f::Cross(a, b).AsNormalized();
}

CPlane CCollisionSurface::GetPlane() const {
  const CUnitVector3f normal(GetNormal());
  return CPlane(CVector3f::Dot(normal, mVertices[0]), normal);
}

// Guessed name
CPlane CCollisionSurface::GetEdgePlane(int edge) const {
  const CUnitVector3f normal(GetNormal());
  const int nextVertex[] = {1, 2, 0};
  const CVector3f& vertex = mVertices[edge];
  const CVector3f edgeDirection = mVertices[nextVertex[edge]] - vertex;
  const CUnitVector3f edgeNormal(CVector3f::Cross(normal, edgeDirection));
  return CPlane(CVector3f::Dot(edgeNormal, vertex), edgeNormal);
}

// Guessed name
bool CCollisionSurface::IsDegenerate() const {
  return mVertices[0] == mVertices[1] || mVertices[1] == mVertices[2] ||
         mVertices[0] == mVertices[2];
}
