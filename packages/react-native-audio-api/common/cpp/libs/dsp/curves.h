#include "./common.h"

#ifndef SIGNALSMITH_DSP_CURVES_H
#define SIGNALSMITH_DSP_CURVES_H

#include <vector>
#include <algorithm> // std::stable_sort

namespace signalsmith {
namespace curves {
	/**	@defgroup Curves Curves
		@brief User-defined mapping functions
		
		@{
		@file
	*/

	/// Linear map for real values.
	template<typename Sample=double>
	class Linear {
		Sample a1, a0;
	public:
		Linear() : Linear(0, 1) {}
		Linear(Sample a0, Sample a1) : a1(a1), a0(a0) {}
		/// Construct by from/to value pairs
		Linear(Sample x0, Sample x1, Sample y0, Sample y1) : a1((x0 == x1) ? 0 : (y1 - y0)/(x1 - x0)), a0(y0 - x0*a1) {}

		Sample operator ()(Sample x) const {
			return a0 + x*a1;
		}
		
		Sample dx() const {
			return a1;
		}
		
		/// Returns the inverse map (with some numerical error)
		Linear inverse() const {
			Sample invA1 = 1/a1;
			return Linear(-a0*invA1, invA1);
		}
	};

	/// A real-valued cubic curve.  It has a "start" point where accuracy is highest.
	template<typename Sample=double>
	class Cubic {
		Sample xStart, a0, a1, a2, a3;
		
		// Only use with y0 != y1
		static inline Sample gradient(Sample x0, Sample x1, Sample y0, Sample y1) {
			return (y1 - y0)/(x1 - x0);
		}
		// Ensure a gradient produces monotonic segments
		static inline void ensureMonotonic(Sample &curveGrad, Sample gradA, Sample gradB) {
			if ((gradA <= 0 && gradB >= 0) || (gradA >= 0 && gradB <= 0)) {
				curveGrad = 0; // point is a local minimum/maximum
			} else {
				if (std::abs(curveGrad) > std::abs(gradA*3)) {
					curveGrad = gradA*3;
				}
				if (std::abs(curveGrad) > std::abs(gradB*3)) {
					curveGrad = gradB*3;
				}
			}
		}
		// When we have duplicate x-values (either side) make up a gradient
		static inline void chooseGradient(Sample &curveGrad, Sample grad1, Sample curveGradOther, Sample y0, Sample y1, bool monotonic) {
			curveGrad = 2*grad1 - curveGradOther;
			if (y0 != y1 && (y1 > y0) != (grad1 >= 0)) { // not duplicate y, but a local min/max
				curveGrad = 0;
			} else if (monotonic) {
				if (grad1 >= 0) {
					curveGrad = std::max<Sample>(0, curveGrad);
				} else {
					curveGrad = std::min<Sample>(0, curveGrad);
				}
			}
		}
	public:
		Cubic() : Cubic(0, 0, 0, 0, 0) {}
		Cubic(Sample xStart, Sample a0, Sample a1, Sample a2, Sample a3) : xStart(xStart), a0(a0), a1(a1), a2(a2), a3(a3) {}
		
		Sample operator ()(Sample x) const {
			x -= xStart;
			return a0 + x*(a1 + x*(a2 + x*a3));
		}
		/// The reference x-value, used as the centre of the cubic expansion
		Sample start() const {
			return xStart;
		}
		/// Differentiate
		Cubic dx() const {
			return {xStart, a1, 2*a2, 3*a3, 0};
		}
		Sample dx(Sample x) const {
			x -= xStart;
			return a1 + x*(2*a2 + x*(3*a3));
		}
		
		/// Cubic segment based on start/end values and gradients
		static Cubic hermite(Sample x0, Sample x1, Sample y0, Sample y1, Sample g0, Sample g1) {
			Sample xScale = 1/(x1 - x0);
			return {
				x0, y0, g0,
				(3*(y1 - y0)*xScale - 2*g0 - g1)*xScale,
				(2*(y0 - y1)*xScale + g0 + g1)*(xScale*xScale)
			};
		}
		
		/** Cubic segment (valid between `x1` and `x2`), which is smooth when applied to an adjacent set of points.
		If `x0 == x1` or `x2 == x3` it will choose a gradient which continues in a quadratic curve, or 0 if the point is a local minimum/maximum.
		*/
		static Cubic smooth(Sample x0, Sample x1, Sample x2, Sample x3, Sample y0, Sample y1, Sample y2, Sample y3, bool monotonic=false) {
			if (x1 == x2) return {0, y1, 0, 0, 0}; // zero-width segment, just return constant

			Sample grad1 = gradient(x1, x2, y1, y2);
			Sample curveGrad1 = grad1;
			bool chooseGrad1 = false;
			if (x0 != x1) { // we have a defined x0-x1 gradient
				Sample grad0 = gradient(x0, x1, y0, y1);
				curveGrad1 = (grad0 + grad1)*Sample(0.5);
				if (monotonic) ensureMonotonic(curveGrad1, grad0, grad1);
			} else if (y0 != y1 && (y1 > y0) != (grad1 >= 0)) {
				curveGrad1 = 0; // set to 0 if it's a min/max
			} else {
				curveGrad1 = 0;
				chooseGrad1 = true;
			}
			Sample curveGrad2;
			if (x2 != x3) { // we have a defined x1-x2 gradient
				Sample grad2 = gradient(x2, x3, y2, y3);
				curveGrad2 = (grad1 + grad2)*Sample(0.5);
				if (monotonic) ensureMonotonic(curveGrad2, grad1, grad2);
			} else {
				chooseGradient(curveGrad2, grad1, curveGrad1, y2, y3, monotonic);
			}
			if (chooseGrad1) {
				chooseGradient(curveGrad1, grad1, curveGrad2, y0, y1, monotonic);
			}
			return hermite(x1, x2, y1, y2, curveGrad1, curveGrad2);
		}
	};
	
	/** Smooth interpolation (optionally monotonic) between points, using cubic segments.
	\diagram{cubic-segments-example.svg,Example curve including a repeated point and an instantaneous jump.  The curve is flat beyond the first/last points.}
	To produce a sharp corner, use a repeated point. The gradient is flat at the edges, unless you use repeated points at the start/end.*/
	template<typename Sample=double>
	class CubicSegmentCurve {
		struct Point {
			Sample x, y;
			Sample lineGrad = 0, curveGrad = 0;
			bool hasCurveGrad = false;

			Point() : Point(0, 0) {}
			Point(Sample x, Sample y) : x(x), y(y) {}

			bool operator <(const Point &other) const {
				return x < other.x;
			}
		};
		std::vector<Point> points;
		Point first{0, 0}, last{0, 0};
		
		std::vector<Cubic<Sample>> _segments{1};
		// Not public because it's only valid inside the bounds
		const Cubic<Sample> & findSegment(Sample x) const {
			// Binary search
			size_t low = 0, high = _segments.size();
			while (true) {
				size_t mid = (low + high)/2;
				if (low == mid) break;
				if (_segments[mid].start() <= x) {
					low = mid;
				} else {
					high = mid;
				}
			}
			return _segments[low];
		}
	public:
		Sample lowGrad = 0;
		Sample highGrad = 0;
	
		/// Clear existing points and segments
		void clear() {
			points.resize(0);
			_segments.resize(0);
			first = last = {0, 0};
		}
	
		/// Add a new point, but does not recalculate the segments.  `corner` just writes the point twice, for convenience.
		CubicSegmentCurve & add(Sample x, Sample y, bool corner=false) {
			points.push_back({x, y});
			if (corner) points.push_back({x, y});
			return *this;
		}
		
		/// Recalculates the segments.
		void update(bool monotonic=false, bool extendGrad=true, Sample monotonicFactor=3) {
			if (points.empty()) add(0, 0);
			std::stable_sort(points.begin(), points.end()); // Ensure ascending order
			_segments.resize(0);

			// Calculate the point-to-point gradients
			for (size_t i = 1; i < points.size(); ++i) {
				auto &prev = points[i - 1];
				auto &next = points[i];
				if (prev.x != next.x) {
					prev.lineGrad = (next.y - prev.y)/(next.x - prev.x);
				} else {
					prev.lineGrad = 0;
				}
			}

			for (auto &p : points) p.hasCurveGrad = false;
			points[0].curveGrad = lowGrad;
			points[0].hasCurveGrad = true;
			points.back().curveGrad = highGrad;
			points.back().hasCurveGrad = true;

			// Calculate curve gradient where we know it
			for (size_t i = 1; i + 1 < points.size(); ++i) {
				auto &p0 = points[i - 1];
				auto &p1 = points[i];
				auto &p2 = points[i + 1];
				if (p0.x != p1.x && p1.x != p2.x) {
					p1.curveGrad = (p0.lineGrad + p1.lineGrad)*Sample(0.5);
					p1.hasCurveGrad = true;
				}
			}
			
			for (size_t i = 1; i < points.size(); ++i) {
				Point &p1 = points[i - 1];
				Point &p2 = points[i];
				if (p1.x == p2.x) continue;
				if (p1.hasCurveGrad) {
					if (!p2.hasCurveGrad) {
						p2.curveGrad = 2*p1.lineGrad - p1.curveGrad;
					}
				} else if (p2.hasCurveGrad) {
					p1.curveGrad = 2*p1.lineGrad - p2.curveGrad;
				} else {
					p1.curveGrad = p2.curveGrad = p1.lineGrad;
				}
			}

			if (monotonic) {
				for (size_t i = 1; i < points.size(); ++i) {
					Point &p1 = points[i - 1];
					Point &p2 = points[i];
					if (p1.x != p2.x) {
						if (p1.lineGrad >= 0) {
							p1.curveGrad = std::max<Sample>(0, std::min(p1.curveGrad, p1.lineGrad*monotonicFactor));
							p2.curveGrad = std::max<Sample>(0, std::min(p2.curveGrad, p1.lineGrad*monotonicFactor));
						} else {
							p1.curveGrad = std::min<Sample>(0, std::max(p1.curveGrad, p1.lineGrad*monotonicFactor));
							p2.curveGrad = std::min<Sample>(0, std::max(p2.curveGrad, p1.lineGrad*monotonicFactor));
						}
					}
				}
			}

			for (size_t i = 1; i < points.size(); ++i) {
				Point &p1 = points[i - 1];
				Point &p2 = points[i];
				if (p1.x != p2.x) {
					_segments.push_back(Segment::hermite(p1.x, p2.x, p1.y, p2.y, p1.curveGrad, p2.curveGrad));
				}
			}
			
			first = points[0];
			last = points.back();
			if (extendGrad && _segments.size()) {
				if (points[0].x != points[1].x || points[0].y == points[1].y) {
					lowGrad = _segments[0].dx(first.x);
				}
				auto &last = points.back(), &last2 = points[points.size() - 1];
				if (last.x != last2.x || last.y == last2.y) {
					highGrad = _segments.back().dx(last.x);
				}
			}
		}
		
		/// Reads a value out from the curve.
		Sample operator()(Sample x) const {
			if (x <= first.x) return first.y + (x - first.x)*lowGrad;
			if (x >= last.x) return last.y + (x - last.x)*highGrad;
			return findSegment(x)(x);
		}

		CubicSegmentCurve dx() const {
			CubicSegmentCurve result{*this};
			result.first.y = lowGrad;
			result.last.y = highGrad;
			result.lowGrad = result.highGrad = 0;
			for (auto &s : result._segments) {
				s = s.dx();
			}
			return result;
		}
		Sample dx(Sample x) const {
			if (x < first.x) return lowGrad;
			if (x >= last.x) return highGrad;
			return findSegment(x).dx(x);
		}

		using Segment = Cubic<Sample>;
		std::vector<Segment> & segments() {
			return _segments;
		}
		const std::vector<Segment> & segments() const {
			return _segments;
		}
	};
	
	/** A warped-range map, based on 1/x
		\diagram{curves-reciprocal-example.svg}*/
	template<typename Sample=double>
	class Reciprocal {
		Sample a, b, c, d; // (a + bx)/(c + dx)
		Reciprocal(Sample a, Sample b, Sample c, Sample d) : a(a), b(b), c(c), d(d) {}
	public:
		/** Decent approximation to the Bark scale
 
		The Bark index goes from 1-24, but this map is valid from approximately 0.25 - 27.5.
		You can get the bandwidth by `barkScale.dx(barkIndex)`.
		\diagram{curves-reciprocal-approx-bark.svg}*/
		static Reciprocal<Sample> barkScale() {
			return {1, 10, 24, 60, 1170, 13500};
		}
		/// Returns a map from 0-1 to the given (non-negative) Hz range.
		static Reciprocal<Sample> barkRange(Sample lowHz, Sample highHz) {
			Reciprocal bark = barkScale();
			Sample lowBark = bark.inverse(lowHz), highBark = bark.inverse(highHz);
			return Reciprocal(lowBark, (lowBark + highBark)/2, highBark).then(bark);
		}

		Reciprocal() : Reciprocal(0, 0.5, 1) {}
		/// If no x-range given, default to the unit range
		Reciprocal(Sample y0, Sample y1, Sample y2) : Reciprocal(0, 0.5, 1, y0, y1, y2) {}
		Reciprocal(Sample x0, Sample x1, Sample x2, Sample y0, Sample y1, Sample y2) {
			Sample kx = (x1 - x0)/(x2 - x1);
			Sample ky = (y1 - y0)/(y2 - y1);
			a = (kx*x2)*y0 - (ky*x0)*y2;
			b = ky*y2 - kx*y0;
			c = kx*x2 - ky*x0;
			d = ky - kx;
		}

		Sample operator ()(double x) const {
			return (a + b*x)/(c + d*x);
		}
		Reciprocal inverse() const {
			return Reciprocal(-a, c, b, -d);
		}
		Sample inverse(Sample y) const {
			return (c*y - a)/(b - d*y);
		}
		Sample dx(Sample x) const {
			Sample l = (c + d*x);
			return (b*c - a*d)/(l*l);
		}
		
		/// Combine two `Reciprocal`s together in sequence
		Reciprocal then(const Reciprocal &other) const {
			return Reciprocal(other.a*c + other.b*a, other.a*d + other.b*b, other.c*c + other.d*a, other.c*d + other.d*b);
		}
	};
	
/** @} */
}} // namespace
#endif // include guard
