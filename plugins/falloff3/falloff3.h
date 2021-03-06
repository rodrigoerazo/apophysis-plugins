/*
    Apophysis Plugin: falloff3

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

//few macros to let me copy-paste MetaCore variation code :)
#define weight			(vp->vvar)
#define param(x)		(vp->var.x)

#define mind(x, x0)		(x<x0?x0:x)
#define maxd(x, x1)		(x>x1?x1:x)
#define sgnd(x)			(x< 0?-1:1)

#define random_double ((((rand()^(rand()<<15))&0xfffffff)*3.72529e-09)-0.5)

// adjustment coefficients
#define scatter_adjust 0.04

inline double log_scale(const double x) { return x == 0 ? 0 : log((fabs(x) + 1.0) * M_E) * sgnd(x) / M_E; }
inline double log_map(const double x) { return x == 0 ? 0 : (M_E + log(x * M_E)) / 4.0 * sgnd(x); }

inline double4 __bt_gaussian(Variation* vp, const double4 v_in, const double4 mul, const double4 random, const double dist) {
	const double sigma	= dist * random.y * M_2PI;
	const double phi	= dist * random.z * M_PI;
	const double rad	= dist * random.x; 

	double sigma_s, sigma_c; fsincos(sigma, &sigma_s, &sigma_c);
	double phi_s, phi_c; fsincos(phi, &phi_s, &phi_c);

	const double4 result = {
		v_in.x + mul.x * rad * sigma_c * phi_c,
		v_in.y + mul.y * rad * sigma_c * phi_s,
		v_in.z + mul.z * rad * sigma_s,
		v_in.c + mul.c * dist * random.c };
	return result;
}
inline double4 __bt_radial(Variation* vp, const double4 v_in, const double4 mul, const double4 random, const double dist) {
	if (v_in.x == 0 && v_in.y == 0 && v_in.z == 0)
		return v_in;

	const double r_in = sqrt(sqr(v_in.x) + sqr(v_in.y) + sqr(v_in.z));

	const double a = mul.y * dist + param(alpha) * dist / sqrt(param(r_max));
	const double b = mul.z * dist;

	const double sigma = asin(v_in.z / r_in) + b * random.z;
	const double phi = atan2(v_in.y, v_in.x) + a * random.y;
	const double r = r_in + mul.x * random.x * dist;

	double sigma_s, sigma_c; fsincos(sigma, &sigma_s, &sigma_c);
	double phi_s, phi_c; fsincos(phi, &phi_s, &phi_c);

	const double4 result = {
		r * sigma_c * phi_c,
		r * sigma_c * phi_s,
		r * sigma_s,
		v_in.c + mul.c * random.c * dist };
	return result;
}
inline double4 __bt_log(Variation* vp, const double4 v_in, const double4 mul, const double4 random, const double dist) {
	const double coeff = param(r_max) <= EPS ? dist : dist + param(alpha) * (log_map(dist) - dist);
	const double4 result = {
		v_in.x + log_map(mul.x) * log_scale(random.x) * coeff,
		v_in.y + log_map(mul.y) * log_scale(random.y) * coeff,
		v_in.z + log_map(mul.z) * log_scale(random.z) * coeff,
		v_in.c + log_map(mul.c) * log_scale(random.c) * coeff };
	return result;
}

inline double __bs_circle(Variation* vp, const double4 v_in, const double3 center) {
	const double distance = sqrt(
		sqr(v_in.x - center.x) +
		sqr(v_in.y - center.y) +
		sqr(v_in.z - center.z));
	return distance;
}
inline double __bs_square(Variation* vp, const double4 v_in, const double3 center) {
	const double distance = mind(fabs(
		v_in.x - center.x), mind(fabs(
		v_in.y - center.y),     (fabs(
		v_in.z - center.z)  
	)));
	return distance;
}

int PluginVarPrepare(Variation* vp)
{
	double4 m = {
		param(fo_mul_x),
		param(fo_mul_y),
		param(fo_mul_z),
		param(fo_mul_c) };
	double3 c = {
		param(fo_c_x),
		param(fo_c_y),
		param(fo_c_z) };

	param(r_max) = scatter_adjust * param(fo_blur_strength);
	param(d) = param(fo_min_distance);

	param(mul) = m;
	param(center) = c;

	param(invert) = param(fo_inv_distance);
	param(type) = param(fo_blur_type);
	param(shape) = param(fo_blur_shape);
	param(alpha) = param(fo_alpha);

    return 1;
}
int PluginVarCalc(Variation* vp)
{
	const double4 v_in =
#ifdef io_post 
	{ *(vp->pFPx), *(vp->pFPy), *(vp->pFPz), *(vp->pColor) };
#else
	{ *(vp->pFTx), *(vp->pFTy), *(vp->pFTz), *(vp->pColor) };
#endif

	const double3 center = param(center);
	const double4 mul = param(mul);

	const double d_0 = param(d);
	const double r_max = param(r_max);

	const double4 random = {
		random_double, random_double,
		random_double, random_double };

	double radius; switch (param(shape)) {
	case bs_circle: 	radius = __bs_circle	(vp, v_in, center); 	break;
	case bs_square: 	radius = __bs_square	(vp, v_in, center); 	break;
	}

	const double dist = mind(((param(invert) != 0 ? mind(1 - radius, 0) : mind(radius, 0)) - d_0) * r_max, 0);

	double4 v_out; switch (param(type)) {
	case bt_gaussian: 	v_out = __bt_gaussian	(vp, v_in, mul, random, dist); 	break;
	case bt_radial: 	v_out = __bt_radial		(vp, v_in, mul, random, dist); 	break;
	case bt_log: 		v_out = __bt_log		(vp, v_in, mul, random, dist); 	break;
	}

	// write back output vector
	#ifdef io_post
		*(vp->pFPx) = v_out.x * weight;
		*(vp->pFPy) = v_out.y * weight;
		*(vp->pFPz) = v_out.z * weight;
	#else
		#ifdef io_pre
			*(vp->pFTx) = v_out.x * weight;
			*(vp->pFTy) = v_out.y * weight;
			*(vp->pFTz) = v_out.z * weight;
		#else
			*(vp->pFPx) += v_out.x * weight;
			*(vp->pFPy) += v_out.y * weight;
			*(vp->pFPz) += v_out.z * weight;
		#endif
	#endif

	// write back output color
	*(vp->pColor) = fabs(fmod(v_out.c, 1.0));

	return 1;
}
