#include <stdbool.h>
#include <math.h>
#include "geometry.h"
#include "glla.h"

//Ported from http://geometrictools.com/Documentation/IntersectionSphereCone.pdf
bool sphere_cone_intersect(struct sphere s, struct cone c)
{
	vec3 u = c.vertex - c.axis * (s.radius*c.sin_rec);
	vec3 d = s.center - u;
	float d_sq = vec3_dot(d, d);
	float e = vec3_dot(c.axis, d);
	if (e > 0 && e*e >= d_sq * c.cos_sq) {
		d = s.center - c.vertex;
		d_sq = vec3_dot(d, d);
		e = -vec3_dot(c.axis, d);
		if (e > 0 && e*e >= d_sq*c.sin_sq)
			return d_sq <= s.radius_sq;
		else
			return true;
	}
	return false;
}

//Ported from http://geomalgorithms.com/a06-_intersect-2.html
//Adapted to give useful parametric s and t as output variables.
//r_start, r_end: Ray start and end (end can just be any point along the ray's length).
//tri: The three vertices that define the triangle we're intersecting.
//intersection: Output variable for the point of intersection.
//par_s, par_t: Output variables for the parametric coordinates of the point of intersection on the triangle.
//par_s is from tri[1] to tri[2], par_t is from tri[0] to tri[1].
const float SMALL_NUM = 0.00000001;
const vec3 vec3_zero = {0, 0, 0};
int ray_tri_intersect_with_parametric(vec3 r_start, vec3 r_end, vec3 tri[3], vec3 *intersection, float *par_s, float *par_t)
{
	vec3 n, r_dir, w0, w;
	float r, a, b;

	vec3 u = tri[2] - tri[1];
	vec3 v = tri[0] - tri[1];
	// vec3 u = tri[1] - tri[0];
	// vec3 v = tri[2] - tri[0];

	n = vec3_cross(u, v);
	if (n.x == 0 && n.y == 0 && n.z == 0)
		return -1;

	r_dir = r_end - r_start;
	w0 = r_start - tri[1];
	a = -vec3_dot(n, w0);
	b = vec3_dot(n, r_dir);
	if (fabs(b) < SMALL_NUM) {
		if (a == 0)
			return 2;
		return 0;
	}

	r = a / b;
	if (r < 0.0)
		return 0;
	
	*intersection = r_start + r * r_dir;

	float uu, uv, vv, wu, wv, D;
	uu = vec3_dot(u, u);
	uv = vec3_dot(u, v);
	vv = vec3_dot(v, v);
	w = *intersection - tri[1];
	wu = vec3_dot(w, u);
	wv = vec3_dot(w, v);
	D = uv * uv - uu * vv;

	float s, t;
	s = (uv * wv - vv * wu) / D;
	t = (uv * wu - uu * wv) / D;
	*par_s = s;
	*par_t = t;
	
	//TODO: Adjust this expression for the new s and t values!
	if (s < 0.0 || s > 1.0 || t < 0.0 || (s + t) > 1.0)
		return 0;

	return 1;
}

int ray_tri_intersect(vec3 r_start, vec3 r_end, vec3 tri[3], vec3 *intersection)
{
	float par_s, par_t;
	return ray_tri_intersect_with_parametric(r_start, r_end, tri, intersection, &par_s, &par_t);
}

