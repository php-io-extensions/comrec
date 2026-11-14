#include "boxes.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

bool pq_intersect(pq_rect a, pq_rect b, pq_rect *out)
{
	int64_t x = a.x > b.x ? a.x : b.x;
	int64_t y = a.y > b.y ? a.y : b.y;
	int64_t ar = a.x + a.w, br = b.x + b.w;
	int64_t ab = a.y + a.h, bb = b.y + b.h;
	int64_t r = ar < br ? ar : br;
	int64_t b_ = ab < bb ? ab : bb;

	if (r <= x || b_ <= y) {
		return false;
	}
	if (out != NULL) {
		out->x = x;
		out->y = y;
		out->w = r - x;
		out->h = b_ - y;
	}
	return true;
}

pq_rect pq_union(pq_rect a, pq_rect b)
{
	pq_rect u;
	int64_t ar = a.x + a.w, br = b.x + b.w;
	int64_t ab = a.y + a.h, bb = b.y + b.h;

	u.x = a.x < b.x ? a.x : b.x;
	u.y = a.y < b.y ? a.y : b.y;
	u.w = (ar > br ? ar : br) - u.x;
	u.h = (ab > bb ? ab : bb) - u.y;
	return u;
}

pq_rect pq_snap(pq_rect r, pq_grain g)
{
	pq_rect s;
	int64_t x1 = ((r.x + r.w + g.unit_w - 1) / g.unit_w) * g.unit_w;
	int64_t y1 = ((r.y + r.h + g.unit_h - 1) / g.unit_h) * g.unit_h;

	s.x = (r.x / g.unit_w) * g.unit_w;
	s.y = (r.y / g.unit_h) * g.unit_h;
	if (x1 > g.limit_w) {
		x1 = g.limit_w;
	}
	if (y1 > g.limit_h) {
		y1 = g.limit_h;
	}
	s.w = x1 - s.x;
	s.h = y1 - s.y;
	return s;
}

bool pq_piece_box(const pq_piece *p, int64_t sw, int64_t sh, pq_rect *out)
{
	double left = floor(p->x0), top = floor(p->y0);
	double right = ceil(p->x1), bottom = ceil(p->y1);
	pq_rect whole;

	/* A side that is not a number, or a rect that starts past the surface or ends before it, covers nothing. */
	if (!(left < (double) sw) || !(top < (double) sh) || !(right > 0.0) || !(bottom > 0.0)) {
		return false;
	}
	whole.x = left > 0.0 ? (int64_t) left : 0;
	whole.y = top > 0.0 ? (int64_t) top : 0;
	whole.w = (right < (double) sw ? (int64_t) right : sw) - whole.x;
	whole.h = (bottom < (double) sh ? (int64_t) bottom : sh) - whole.y;
	if (whole.w <= 0 || whole.h <= 0) {
		return false;
	}
	return pq_intersect(whole, p->clip, out);
}

void pq_rects_init(pq_rects *l)
{
	l->v = NULL;
	l->n = 0;
	l->cap = 0;
}

void pq_rects_free(pq_rects *l)
{
	free(l->v);
	pq_rects_init(l);
}

void pq_rects_push(pq_rects *l, pq_rect r)
{
	if (l->n == l->cap) {
		uint32_t cap = l->cap == 0 ? 16 : l->cap * 2;
		pq_rect *v = realloc(l->v, (size_t) cap * sizeof(pq_rect));

		if (v == NULL) {
			abort();
		}
		l->v = v;
		l->cap = cap;
	}
	l->v[l->n++] = r;
}

void pq_merge_one(pq_rects *merged, pq_rect box, pq_grain g)
{
	bool grew;

	box = pq_snap(box, g);
	do {
		uint32_t kept = 0;

		grew = false;
		/* One pass in order: a rect the box overlaps joins it and leaves the list; the rest close up. */
		for (uint32_t i = 0; i < merged->n; i++) {
			if (pq_intersect(merged->v[i], box, NULL)) {
				box = pq_snap(pq_union(box, merged->v[i]), g);
				grew = true;
			} else {
				merged->v[kept++] = merged->v[i];
			}
		}
		merged->n = kept;
	} while (grew);
	pq_rects_push(merged, box);
}

void pq_merge(const pq_rects *in, pq_rects *out, pq_grain g)
{
	for (uint32_t i = 0; i < in->n; i++) {
		pq_merge_one(out, in->v[i], g);
	}
}
