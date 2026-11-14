/*
 * Whole-pixel rects: a command's box, and boxes merged until no two overlap.
 * Plain C, no Zend.
 */
#ifndef PQ_BOXES_H
#define PQ_BOXES_H

#include <stdbool.h>
#include <stdint.h>

/* The furthest a transformed coordinate may land: 2^24. */
#define PQ_LIMIT 16777216.0

typedef struct {
	int64_t x, y, w, h;
} pq_rect;

/* How far a command's pixels reach before they are held to whole pixels, and the clip they stay inside. */
typedef struct {
	double x0, y0, x1, y1;
	pq_rect clip;
} pq_piece;

/* The smallest rect a surface repaints: units wide and high, and the size boxes are held to. */
typedef struct {
	int64_t unit_w, unit_h, limit_w, limit_h;
} pq_grain;

typedef struct {
	pq_rect *v;
	uint32_t n, cap;
} pq_rects;

/* The overlap of a and b in out; false when they share no pixel. */
bool pq_intersect(pq_rect a, pq_rect b, pq_rect *out);

/* The smallest rect holding both. */
pq_rect pq_union(pq_rect a, pq_rect b);

/* Grown outward to whole units, and held to the grain's limit on the right and the bottom. */
pq_rect pq_snap(pq_rect r, pq_grain g);

/*
 * The whole pixels a piece covers on a surface sw by sh, inside its clip:
 * left and top rounded down and held to 0, right and bottom rounded up and
 * held to the surface. False when that is no pixel.
 */
bool pq_piece_box(const pq_piece *p, int64_t sw, int64_t sh, pq_rect *out);

void pq_rects_init(pq_rects *l);
void pq_rects_free(pq_rects *l);
void pq_rects_push(pq_rects *l, pq_rect r);

/*
 * Add box to a merged list: snapped, then joined with every rect it overlaps,
 * again and again until it overlaps none, and put last. The list stays in the
 * order its rects were put.
 */
void pq_merge_one(pq_rects *merged, pq_rect box, pq_grain g);

/* in merged from nothing into out, in order. out starts empty. */
void pq_merge(const pq_rects *in, pq_rects *out, pq_grain g);

#endif
