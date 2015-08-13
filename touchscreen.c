/*
 * Routines for touchscreen calibration and coordinate transformation into
 * display coordinates.
 *
 * References:
 * http://www.maximintegrated.com/en/app-notes/index.mvp/id/5296
 * http://mathworld.wolfram.com/MatrixInverse.html
 */

#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct point {
	float	 x;
	float	 y;
};

float
det22(float a, float b, float c, float d)
{
	return a*d - b*c;
}

/*
 * Compute determinant of 3x3 matrix 'm'.
 */
float
det33(const float m[3][3])
{
	return
	    (m[0][0] * det22(m[1][1], m[1][2], m[2][1], m[2][2])) -
	    (m[0][1] * det22(m[1][0], m[1][2], m[2][0], m[2][2])) +
	    (m[0][2] * det22(m[1][0], m[1][1], m[2][0], m[2][1]));
}

/* 
 * Compute inverse matrix of 3x3 matrix 'in'.
 * Return true if successful.
 */
bool
invert33(const float in[3][3], float out[3][3])
{
	float	 d;
	float	 d_i;

	d = det33(in);
	if (d == 0)
		return false;
	d_i = 1.0 / d;

#define D(							\
		a_i, a_j,  b_i, b_j,				\
		c_i, c_j,  d_i, d_j				\
	)							\
		det22(in[a_i-1][a_j-1], in[b_i-1][b_j-1],	\
		    in[c_i-1][c_j-1], in[d_i-1][d_j-1])

	float	 a[3][3] = {
		/* row 0 */
		{
			D(2,2, 2,3,
			  3,2, 3,3),
			D(1,3, 1,2,
			  3,3, 3,2),
			D(1,2, 1,3,
			  2,2, 2,3)
		},
		/* row 1 */
		{
			D(2,3, 2,1,
			  3,3, 3,1),
			D(1,1, 1,3,
			  3,1, 3,3),
			D(1,3, 1,1,
			  2,3, 2,1)
		},
		/* row 2 */
		{
			D(2,1, 2,2,
			  3,1, 3,2),
			D(1,2, 1,1,
			  3,2, 3,1),
			D(1,1, 1,2,
			  2,1, 2,2)
		},
	};

#undef D

	int	 i, j;
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			out[i][j] = d_i * a[i][j];

	return true;
}

/*
 * Compute product of 3x3 matrix 'm' and 3x1 vector 'v'.
 */
void
dot333(const float m[3][3], const float v[3], float out[3])
{
	int	 i;

	for (i = 0; i < 3; i++)
		out[i] = v[0] * m[i][0] + v[1] * m[i][1] + v[2] * m[i][2];
}

bool
compute_affine_xform_matrix(struct point disp_pts[3], struct point ts_pts[3], float out[2][3])
{
	int	 i;
	float	 x_ts[3];
	float	 y_ts[3];
	float	 x_disp[3];
	float	 y_disp[3];

	for (i = 0; i < 3; i++) {
		x_ts[i] = ts_pts[i].x;
		y_ts[i] = ts_pts[i].y;
		x_disp[i] = disp_pts[i].x;
		y_disp[i] = disp_pts[i].y;
	}

	float	 m[3][3] = {
		{ x_ts[0], y_ts[0], 1 },
		{ x_ts[1], y_ts[1], 1 },
		{ x_ts[2], y_ts[2], 1 },
	};
	float	m_i[3][3];

	if (!invert33(m, m_i))
		return false;

	dot333(m_i, x_disp, &out[0][0]);
	dot333(m_i, y_disp, &out[1][0]);

	return true;
}

/*
 * Transform coordinates from touchscreen space into display space.
 */
void
affine_xform(const float m[2][3], float x_ts, float y_ts, float *x_disp, float *y_disp)
{
	*x_disp = m[0][0] * x_ts + m[0][1] * y_ts + m[0][2];
	*y_disp = m[1][0] * x_ts + m[1][1] * y_ts + m[1][2];
}

int
main(int argc, char **argv)
{
	/*
	 * Display coordinates of 3 calibration points.
	 */
	struct point disp_points[] = {
		{ 229, 65 },
		{ 285, 476 },
		{ 58, 681 }
	};

	/*
	 * Coordinates received from TS which correspond to the above 3
	 * calibration points.
	 */
	struct point ts_points[] = {
		{ 242, 70 },
		{ 190, 381 },
		{ 414, 546 }
	};

	float	 affine_m[2][3] = {
		{ 1.0, 0.0, 0.0 },
		{ 0.0, 1.0, 0.0 }
	};

	if (!compute_affine_xform_matrix(disp_points, ts_points, affine_m))
		errx(1, "failed to compute affine transformation matrix");

	printf(
	    "matrix = {\n"
	    "  { %f %f %f }\n"
	    "  { %f %f %f }\n"
	    "}\n",
	    affine_m[0][0], affine_m[0][1], affine_m[0][2],
	    affine_m[1][0], affine_m[1][1], affine_m[1][2]
	);

	/*
	 * Now feed the touchscreen coordinates to transformation function. On
	 * output, we should get the display coordinates (the disp_points
	 * values).
	 */
	for (int i = 0; i < 3; i++) {
		float x, y;
		affine_xform(affine_m, ts_points[i].x, ts_points[i].y, &x, &y);
		printf("disp{ %f, %f }, ts{ %f, %f } --> { %f, %f }\n",
		    disp_points[i].x, disp_points[i].y,
		    ts_points[i].x, ts_points[i].y,
		    x, y);
	}

	return 0;
}
