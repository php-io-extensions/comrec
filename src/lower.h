/*
 * A command in a simpler form, for a drawer whose library cannot draw it as it is: outlines as straight-sided
 * contours, a stroke as the area it covers, an area as trapezoids, trapezoids as triangles. Every number is in frame
 * pixels already. Plain C, no Zend.
 */
#ifndef PQ_LOWER_H
#define PQ_LOWER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A stroke's corner is mitered up to this miter, beveled past it. */
#define PQ_MITER_LIMIT 4.0

/* Contours: x, y of every point, contour after contour, and how many points each has. */
typedef struct {
	double *xy;
	uint32_t *counts;
	uint32_t points, contours, open;   /* open: points in the contour being added */
	uint32_t xy_cap, counts_cap;
} pq_shape;

/* Doubles, a fixed number to an item: 6 to a trapezoid, 6 to a triangle. */
typedef struct {
	double *v;
	size_t n, cap;
} pq_doubles;

void pq_shape_init(pq_shape *s);
void pq_shape_free(pq_shape *s);
bool pq_shape_point(pq_shape *s, double x, double y);
bool pq_shape_end(pq_shape *s);

void pq_doubles_free(pq_doubles *d);

/* One contour each: an ellipse; a rounded box clockwise from its top-left corner (radii tl, tr, br, bl). */
bool pq_lower_ellipse(pq_shape *s, double cx, double cy, double rx, double ry);
bool pq_lower_outline(pq_shape *s, double x, double y, double w, double h, const double radii[4]);

/* The area a stroke of width along n points covers: contours all wound one way, to fill non-zero. */
bool pq_lower_stroke(pq_shape *s, const double *points, uint32_t n, double width, bool closed);

/* The area inside s under the rule as trapezoids: top, bottom, left at top, left at bottom, right at top, right at bottom. */
bool pq_lower_trapezoids(const pq_shape *s, bool even_odd, pq_doubles *out);

/* Each trapezoid as up to two triangles, x0, y0, x1, y1, x2, y2, without the ones with no area. */
bool pq_lower_triangles(const double *traps, size_t count, pq_doubles *out);

#endif
