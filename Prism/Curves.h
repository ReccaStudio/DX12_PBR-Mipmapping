#pragma once

#include <vector>
#include <DirectXMath.h>

#include "CubicPolynomial.h"

using namespace DirectX;

enum SplineType { Normal, Clamped, Closed };

template <class T>
inline T Modulo(T x, T m) { return (x % m + m) % m; }

// Precise method, which guarantees v = v1 when t = 1.
inline float Lerp(float v0, float v1, float t) {
	return (1 - t) * v0 + t * v1;
}

class CubicBezier
{
public:

	// Control Vertices
	std::vector<XMFLOAT3> CV;

	// Polynomials for interpolation of the x coordinate
	std::vector<CubicPolynomial> Px;

	// Polynomials for interpolation of the y coordinate
	std::vector<CubicPolynomial> Py;
	
	// Polynomials for interpolation of the z coordinate
	std::vector<CubicPolynomial> Pz;

	// Contruct an emtpy curve
	CubicBezier();

	// Creates a new polynimal for a segment from four control vertices
	CubicPolynomial CalculateCubicPolynomial(float p0, float p1, float p2, float p3) const;
	QuadraticPolynomial CalculateQuadraticPolynomial(float p0, float p1, float p2) const;

	void Rebuild();

	// Calculates the samples and returns them for drawing
	XMFLOAT3 Value(float t) const;

	// Calculates the samples and returns them for drawing
	XMFLOAT3 Derivative(float t) const;

	XMFLOAT3 Center(XMFLOAT3 p1, XMFLOAT3 p2);
};


class CardinalCurve
{
public:
	float tension;

	bool closed;

	// Control Vertices
	std::vector<XMFLOAT3> CV;

	// Polynomials for interpolation of the x coordinate
	std::vector<CubicPolynomial> Px;

	// Polynomials for interpolation of the y coordinate
	std::vector<CubicPolynomial> Py;

	// Polynomials for interpolation of the z coordinate
	std::vector<CubicPolynomial> Pz;

	// Contruct an emtpy curve
	CardinalCurve();

	// Creates a new polynimal for a segment from four control vertices
	CubicPolynomial CalculatePolynomial(float p0, float p1, float p2, float p3) const;

	void Rebuild();

	// Rebuilds the curve
	void RebuildOpen();

	void RebuildClosed();

	// Calculates the samples and returns them for drawing
	XMFLOAT3 Value(float t) const;

	// Calculates the samples and returns them for drawing
	XMFLOAT3 Derivative(float t) const;
};




class BSpline
{
public:
	bool closed;

	// Control Vertices
	std::vector<XMFLOAT3> CV;

	std::vector<int> knots;

	// Polynomials for interpolation of the x coordinate
	std::vector<CubicPolynomial> Px;

	// Polynomials for interpolation of the y coordinate
	std::vector<CubicPolynomial> Py;

	// Polynomials for interpolation of the z coordinate
	std::vector<CubicPolynomial> Pz;

	int count;

	// Contruct an emtpy curve
	BSpline();

	// Creates a new polynimal for a segment from four control vertices
	// CubicPolynomial CalculatePolynomial(float p0, float p1, float p2, float p3) const;

	void Rebuild();

	void CalculatePolys();

	// Rebuilds the curve
	// void RebuildOpen();

	// void RebuildClosed();

	// Calculates the samples and returns them for drawing
	XMFLOAT3 Value(float t) const;

	// Calculates the samples and returns them for drawing
	XMFLOAT3 Derivative(float t) const;

};