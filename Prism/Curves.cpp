#include "Curves.h"

////////////////////////////////////////////////////////////////////////////////
// Contruct an emtpy curve
////////////////////////////////////////////////////////////////////////////////
CardinalCurve::CardinalCurve()
{
	tension = 0.0f;
	closed = false;
}

////////////////////////////////////////////////////////////////////////////////
// Creates a new polynimal for a segment from four control vertices
////////////////////////////////////////////////////////////////////////////////
CubicPolynomial CardinalCurve::CalculatePolynomial(float p0, float p1, float p2, float p3) const
{
	float p1Der = 0.5f * (1 - tension) * (p2 - p0);
	float p2Der = 0.5f * (1 - tension) * (p3 - p1);
	//
	float a = 2 * p1 + p1Der - 2 * p2 + p2Der;
	float b = -3 * p1 - 2 * p1Der + 3 * p2 - p2Der;
	float c = p1Der;
	float d = p1;
	//
	return CubicPolynomial(a, b, c, d);
}

void CardinalCurve::Rebuild()
{
	if (closed)
		RebuildClosed();
	else
		RebuildOpen();
}

// Rebuilds the curve
void CardinalCurve::RebuildOpen()
{
	// Minimum for creating a segment
	if (CV.size() < 2)
		return;

	Px.clear();
	Py.clear();
	Pz.clear();

	int start = 0; //CV.Count - 2;// Px.Count;
	auto end = CV.size() - 1;

	// Got through all the CVs
	for (int i = start; i < end; i++)
	{
		//int i = CV.Count - 2;
		int prev = (i - 1) < 0 ? 0 : (i - 1);
		int next = i + 1;
		int last = (i + 2) > (CV.size() - 1) ? (i + 1) : (i + 2);

		float p0 = CV[prev].x;
		float p1 = CV[i].x;
		float p2 = CV[next].x;
		float p3 = CV[last].x;
		//
		Px.push_back(CalculatePolynomial(p0, p1, p2, p3));

		p0 = CV[prev].y;
		p1 = CV[i].y;
		p2 = CV[next].y;
		p3 = CV[last].y;
		//
		Py.push_back(CalculatePolynomial(p0, p1, p2, p3));

		p0 = CV[prev].z;
		p1 = CV[i].z;
		p2 = CV[next].z;
		p3 = CV[last].z;
		//
		Pz.push_back(CalculatePolynomial(p0, p1, p2, p3));
	}
}

void CardinalCurve::RebuildClosed()
{
	// Minimum for creating a segment
	if (CV.size() < 2)
		return;

	Px.clear();
	Py.clear();
	Pz.clear();

	int start = 0;
	int end = CV.size();

	// Got through all the CVs
	for (int i = start; i < end; i++)
	{
		int curr = i;
		int prev = Modulo(i - 1, end);
		int next = Modulo(i + 1, end);
		int last = Modulo(i + 2, end);
		//
		XMFLOAT3 p0 = CV[prev];
		XMFLOAT3 p1 = CV[curr];
		XMFLOAT3 p2 = CV[next];
		XMFLOAT3 p3 = CV[last];
		//
		Px.push_back(CalculatePolynomial(p0.x, p1.x, p2.x, p3.x));
		Py.push_back(CalculatePolynomial(p0.y, p1.y, p2.y, p3.y));
		Pz.push_back(CalculatePolynomial(p0.z, p1.z, p2.z, p3.z));
	}
}

// Calculates the samples and returns them for drawing
XMFLOAT3 CardinalCurve::Value(float t) const
{
	auto count = Px.size();

	if (count == 0)
		return XMFLOAT3();

	if (closed)
	{
		if (t < 0.0f) t = 1.0f + t;
		if (t >= 1.0f) t = t - 1.0f;
	}
	else
	{
		if (t <= 0.0f) t = 0.0f;
		if (t >= 1.0f) t = 0.99999f;
	}

	int poly = (int)floorf(t * count);
	t = t - ((float)poly / count);
	t *= count;

	float x = Px[poly].Value(t);
	float y = Py[poly].Value(t);
	float z = Pz[poly].Value(t);

	return XMFLOAT3(x, y, z);
}

// Calculates the samples and returns them for drawing
XMFLOAT3 CardinalCurve::Derivative(float t) const
{
	int count = Px.size();

	if (count == 0)
		return XMFLOAT3();

	if (t <= 0.0f) t = 0.0f;
	if (t >= 1.0f) t = 0.999f;

	auto poly = floor(t * count);
	t = t - (poly / count);
	t *= count;

	float x = Px[poly].FirstDerivate(t);
	float y = Py[poly].FirstDerivate(t);
	float z = Pz[poly].FirstDerivate(t);

	return XMFLOAT3(x, y, z);
}



/////////////////////////////////////////
// BEZIER CURVES
////////////////////////////////////////

CubicBezier::CubicBezier()
{
}

CubicPolynomial CubicBezier::CalculateCubicPolynomial(float p0, float p1, float p2, float p3) const
{
	float a = p3 - (3 * p2) + (3 * p1) - p0;
	float b = (3 * p2) - (6 * p1) + (3 * p0);
	float c = (3 * p1) - (3 * p0);
	float d = p0;

	return CubicPolynomial(a, b, c, d);
}

QuadraticPolynomial CubicBezier::CalculateQuadraticPolynomial(float p0, float p1, float p2) const
{
	float a = p0 - (2 * p1) + p2;
	float b = (2 * p1) - (2 * p0);
	float c = p0;

	return QuadraticPolynomial(a, b, c);
}

XMFLOAT3 CubicBezier::Center(XMFLOAT3 p1, XMFLOAT3 p2)
{
	return XMFLOAT3((p1.x + p2.x) / 2,
					(p1.y + p2.y) / 2,
					(p1.z + p2.z) / 2);
}


void CubicBezier::Rebuild()
{
	if (CV.size() < 4)
		false;

	Px.clear();
	Py.clear();
	Pz.clear();

	int start = 0; //CV.Count - 2;// Px.Count;
	auto end = CV.size() - 1;

	XMFLOAT3 p0 = CV[0];
	XMFLOAT3 p1 = CV[1];
	XMFLOAT3 p2 = CV[2];
	XMFLOAT3 p3 = CV[3];

	Px.push_back(CalculateCubicPolynomial(p0.x, p1.x, p2.x, p3.x));

	Py.push_back(CalculateCubicPolynomial(p0.y, p1.y, p2.y, p3.y));

	Pz.push_back(CalculateCubicPolynomial(p0.z, p1.z, p2.z, p3.z));
}

XMFLOAT3 CubicBezier::Value(float t) const
{
	float x = Px[0].Value(t);
	float y = Py[0].Value(t);
	float z = Pz[0].Value(t);

	return XMFLOAT3(x, y, z);
}

XMFLOAT3 CubicBezier::Derivative(float t) const
{
	float x = Px[0].FirstDerivate(t);
	float y = Py[0].FirstDerivate(t);
	float z = Pz[0].FirstDerivate(t);

	return XMFLOAT3(x, y, z);
}


/////////////////////////////////////////
// B-SPLINE
////////////////////////////////////////

BSpline::BSpline()
	: closed(false)
	, count(0)
{
}

void BSpline::Rebuild()
{
	count = CV.size();
	Px.clear();
	Py.clear();
	Pz.clear();


	// Create a clamped curve
	knots.resize(count + 6);
	// Px.resize(count + 3);
	// Py.resize(count + 3);
	// Pz.resize(count + 3);;
	knots[0] = 0;
	knots[1] = 0;
	knots[2] = 0;
	for (int i = 3; i < count + 3; i++)
		knots[i] = i - 3;
	knots[count + 3] = count - 1;
	knots[count + 4] = count - 1;
	knots[count + 5] = count - 1;

	CalculatePolys();
}

void BSpline::CalculatePolys()
{
	for (int i = 0; i < knots.size() - 3; i++)
	{
		Px.push_back(CubicPolynomial(
			(-CV[knots[i]].x + 3.0f * CV[knots[i + 1]].x - 3.0f * CV[knots[i + 2]].x + CV[knots[i + 3]].x) / 6.0f,
			(3.0f * CV[knots[i]].x - 6.0f * CV[knots[i + 1]].x + 3.0f * CV[knots[i + 2]].x) / 6.0f,
			(-3.0f * CV[knots[i]].x + 3.0f * CV[knots[i + 2]].x) / 6.0f,
			(CV[knots[i]].x + 4.0f * CV[knots[i + 1]].x + CV[knots[i + 2]].x) / 6.0f));

		Py.push_back(CubicPolynomial(
			(-CV[knots[i]].y + 3.0f * CV[knots[i + 1]].y - 3.0f * CV[knots[i + 2]].y + CV[knots[i + 3]].y) / 6.0f,
			(3.0f * CV[knots[i]].y - 6.0f * CV[knots[i + 1]].y + 3.0f * CV[knots[i + 2]].y) / 6.0f,
			(-3.0f * CV[knots[i]].y + 3.0f * CV[knots[i + 2]].y) / 6.0f,
			(CV[knots[i]].y + 4.0f * CV[knots[i + 1]].y + CV[knots[i + 2]].y) / 6.0f));

		Pz.push_back(CubicPolynomial(
			(-CV[knots[i]].z + 3.0f * CV[knots[i + 1]].z - 3.0f * CV[knots[i + 2]].z + CV[knots[i + 3]].z) / 6.0f,
			(3.0f * CV[knots[i]].z - 6.0f * CV[knots[i + 1]].z + 3.0f * CV[knots[i + 2]].z) / 6.0f,
			(-3.0f * CV[knots[i]].z + 3.0f * CV[knots[i + 2]].z) / 6.0f,
			(CV[knots[i]].z + 4.0f * CV[knots[i + 1]].z + CV[knots[i + 2]].z) / 6.0f));
	}
}

XMFLOAT3 BSpline::Value(float t) const
{
	auto count = Px.size();

	if (count == 0)
		return XMFLOAT3();

	if (t <= 0.0f) t = 0.0f;
	if (t >= 1.0f) t = 0.99999f;

	int poly = (int)floorf(t * count);
	t = t - ((float)poly / count);
	t *= count;

	float x = Px[poly].Value(t);
	float y = Py[poly].Value(t);
	float z = Pz[poly].Value(t);

	return XMFLOAT3(x, y, z);
}

XMFLOAT3 BSpline::Derivative(float t) const
{
	int count = Px.size();

	if (count == 0)
		return XMFLOAT3();

	if (t <= 0.0f) t = 0.001f;
	if (t >= 1.0f) t = 0.999f;

	auto poly = floor(t * count);
	t = t - (poly / count);
	t *= count;

	float x = Px[poly].FirstDerivate(t);
	float y = Py[poly].FirstDerivate(t);
	float z = Pz[poly].FirstDerivate(t);

	return XMFLOAT3(x, y, z);
}
