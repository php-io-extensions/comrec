/*
 * Lowering, step for step as topside's Surface\Rendering\CommandRecorders\Lowering: the same operations in the same
 * order, so the two give the same numbers bit for bit. Plain C, no Zend.
 */
#include "lower.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* A band thinner than this is not cut again where its edges cross. */
#define PQ_THINNEST 1e-9

/* ------------------------------------------------------------------ storage */

void pq_shape_init(pq_shape *s)
{
	memset(s, 0, sizeof(*s));
}

void pq_shape_free(pq_shape *s)
{
	free(s->xy);
	free(s->counts);
	memset(s, 0, sizeof(*s));
}

bool pq_shape_point(pq_shape *s, double x, double y)
{
	if (s->points == s->xy_cap) {
		uint32_t cap = s->xy_cap ? s->xy_cap * 2 : 64;
		double *grown = realloc(s->xy, (size_t) cap * 2 * sizeof(double));
		if (grown == NULL) {
			return false;
		}
		s->xy = grown;
		s->xy_cap = cap;
	}
	s->xy[2 * s->points] = x;
	s->xy[2 * s->points + 1] = y;
	s->points++;
	s->open++;
	return true;
}

/* Ends the contour being added; a contour with no points is not kept. */
bool pq_shape_end(pq_shape *s)
{
	if (s->open == 0) {
		return true;
	}
	if (s->contours == s->counts_cap) {
		uint32_t cap = s->counts_cap ? s->counts_cap * 2 : 8;
		uint32_t *grown = realloc(s->counts, (size_t) cap * sizeof(uint32_t));
		if (grown == NULL) {
			return false;
		}
		s->counts = grown;
		s->counts_cap = cap;
	}
	s->counts[s->contours++] = s->open;
	s->open = 0;
	return true;
}

void pq_doubles_free(pq_doubles *d)
{
	free(d->v);
	memset(d, 0, sizeof(*d));
}

static bool pq_doubles_add(pq_doubles *d, const double *values, size_t count)
{
	if (d->n + count > d->cap) {
		size_t cap = d->cap ? d->cap * 2 : 96;
		while (cap < d->n + count) {
			cap *= 2;
		}
		double *grown = realloc(d->v, cap * sizeof(double));
		if (grown == NULL) {
			return false;
		}
		d->v = grown;
		d->cap = cap;
	}
	memcpy(d->v + d->n, values, count * sizeof(double));
	d->n += count;
	return true;
}

/* ------------------------------------------------------------------ outlines */

/* As the recorder's: how many straight sides keep a circle of this radius within 0.1 of a pixel, 12..1024. */
static int pq_sides(double radius)
{
	double sides = ceil(M_PI * sqrt(radius / (2 * 0.1)) * 1.01);

	if (!(sides < 1024.0)) {
		sides = 1024.0;
	}
	if (!(sides > 12.0)) {
		sides = 12.0;
	}
	return (int) sides;
}

bool pq_lower_ellipse(pq_shape *s, double cx, double cy, double rx, double ry)
{
	int sides = pq_sides(rx < ry ? ry : rx);

	for (int i = 0; i < sides; i++) {
		double angle = 2 * M_PI * (double) i / (double) sides;
		if (!pq_shape_point(s, cx + rx * cos(angle), cy + ry * sin(angle))) {
			return false;
		}
	}
	return pq_shape_end(s);
}

bool pq_lower_outline(pq_shape *s, double x, double y, double w, double h, const double radii[4])
{
	const double corners[4][4] = {
		{ x + radii[0], y + radii[0], radii[0], M_PI },
		{ x + w - radii[1], y + radii[1], radii[1], 1.5 * M_PI },
		{ x + w - radii[2], y + h - radii[2], radii[2], 0.0 },
		{ x + radii[3], y + h - radii[3], radii[3], 0.5 * M_PI },
	};

	for (int k = 0; k < 4; k++) {
		double cx = corners[k][0], cy = corners[k][1], r = corners[k][2], from = corners[k][3];
		if (r <= 0) {
			if (!pq_shape_point(s, cx, cy)) {
				return false;
			}
			continue;
		}
		int steps = (pq_sides(r) + 3) / 4;
		for (int i = 0; i <= steps; i++) {
			double angle = from + 0.5 * M_PI * (double) i / (double) steps;
			if (!pq_shape_point(s, cx + r * cos(angle), cy + r * sin(angle))) {
				return false;
			}
		}
	}
	return pq_shape_end(s);
}

/* ------------------------------------------------------------------ strokes */

/*
 * A stroke's left side along n points (pts, x then y), into the contour being added, with its corners: an outer
 * corner mitered while 1 / cos of half its turn is at most PQ_MITER_LIMIT, else beveled; an inner corner, and a way
 * straight back, through the corner's point. normals and dirs have room for a side each.
 */
static bool pq_side(pq_shape *s, const double *pts, uint32_t n, double half, bool closed, double *normals, double *dirs)
{
	uint32_t sides = closed ? n : n - 1, from = closed ? 0 : 1, to = closed ? n : n - 1;

	for (uint32_t i = 0; i < sides; i++) {
		double ax = pts[2 * i], ay = pts[2 * i + 1];
		double bx = pts[2 * ((i + 1) % n)], by = pts[2 * ((i + 1) % n) + 1];
		double dx = bx - ax, dy = by - ay;
		double length = sqrt(dx * dx + dy * dy);

		normals[2 * i] = -dy / length * half;
		normals[2 * i + 1] = dx / length * half;
		dirs[2 * i] = dx;
		dirs[2 * i + 1] = dy;
	}
	if (!closed && !pq_shape_point(s, pts[0] + normals[0], pts[1] + normals[1])) {
		return false;
	}
	for (uint32_t j = from; j < to; j++) {
		uint32_t before = (j - 1 + sides) % sides, after = j % sides;
		double px = pts[2 * j], py = pts[2 * j + 1];
		double n0x = normals[2 * before], n0y = normals[2 * before + 1], n1x = normals[2 * after], n1y = normals[2 * after + 1];
		double d0x = dirs[2 * before], d0y = dirs[2 * before + 1], d1x = dirs[2 * after], d1y = dirs[2 * after + 1];
		double turn = d0x * d1y - d0y * d1x;
		bool back = d0x * d1x + d0y * d1y < 0;

		if (!pq_shape_point(s, px + n0x, py + n0y)) {
			return false;
		}
		if (turn > 0) {
			/* turning towards this side: the inner corner, through the corner's point */
			if (!pq_shape_point(s, px, py)) {
				return false;
			}
		} else if (turn < 0) {
			/* turning away from it: the outer corner, mitered while the miter is short enough */
			double sx = n0x + n1x, sy = n0y + n1y;
			double cosine = sqrt(sx * sx + sy * sy) / (2 * half);
			if (cosine > 0 && 1 / cosine <= PQ_MITER_LIMIT) {
				double k = 1 / (2 * cosine * cosine);
				if (!pq_shape_point(s, px + sx * k, py + sy * k)) {
					return false;
				}
			}
		} else if (back) {
			/* straight back the way it came: around the end, through the point */
			if (!pq_shape_point(s, px, py)) {
				return false;
			}
		}
		if ((turn != 0.0 || back) && !pq_shape_point(s, px + n1x, py + n1y)) {
			return false;
		}
	}
	if (!closed) {
		uint32_t last = sides - 1;
		if (!pq_shape_point(s, pts[2 * (n - 1)] + normals[2 * last], pts[2 * (n - 1) + 1] + normals[2 * last + 1])) {
			return false;
		}
	}
	return true;
}

bool pq_lower_stroke(pq_shape *s, const double *points, uint32_t count, double width, bool closed)
{
	double *pts, *back, *normals, *dirs, half;
	uint32_t n = 0, sides;
	bool ok;

	if (count == 0) {
		return true;
	}
	pts = malloc((size_t) count * 2 * sizeof(double));
	if (pts == NULL) {
		return false;
	}
	for (uint32_t i = 0; i < count; i++) {
		double x = points[2 * i], y = points[2 * i + 1];
		if (n == 0 || pts[2 * (n - 1)] != x || pts[2 * (n - 1) + 1] != y) {
			pts[2 * n] = x;
			pts[2 * n + 1] = y;
			n++;
		}
	}
	if (closed && n > 1 && pts[0] == pts[2 * (n - 1)] && pts[1] == pts[2 * (n - 1) + 1]) {
		n--;
	}
	if (n < 2 || !(width > 0)) {
		free(pts);
		return true;
	}
	half = width / 2;
	sides = closed ? n : n - 1;
	back = malloc((size_t) n * 2 * sizeof(double));
	normals = malloc((size_t) sides * 2 * sizeof(double));
	dirs = malloc((size_t) sides * 2 * sizeof(double));
	if (back == NULL || normals == NULL || dirs == NULL) {
		free(pts);
		free(back);
		free(normals);
		free(dirs);
		return false;
	}
	for (uint32_t i = 0; i < n; i++) {
		back[2 * i] = pts[2 * (n - 1 - i)];
		back[2 * i + 1] = pts[2 * (n - 1 - i) + 1];
	}

	/* the left side going forward, then the left side of the way back: one contour open, two closed */
	ok = pq_side(s, pts, n, half, closed, normals, dirs) && (!closed || pq_shape_end(s))
		&& pq_side(s, back, n, half, closed, normals, dirs) && pq_shape_end(s);

	free(pts);
	free(back);
	free(normals);
	free(dirs);
	return ok;
}

/* ------------------------------------------------------------------ trapezoids */

typedef struct {
	double x0, y0, x1, y1;
	int direction;
} pq_edge;

/* An edge in a band: where it is at the top, at the bottom and halfway down. */
typedef struct {
	double top, bottom, middle;
	uint32_t index;
	int direction;
} pq_active;

static int pq_by_height(const void *a, const void *b)
{
	double x = *(const double *) a, y = *(const double *) b;
	return x < y ? -1 : (x > y ? 1 : 0);
}

/* An edge's start height and index: the edges are sorted by these, so the sort needs nothing outside it. */
typedef struct {
	double y0;
	uint32_t index;
} pq_start;

static int pq_by_start(const void *a, const void *b)
{
	const pq_start *p = a, *q = b;
	if (p->y0 != q->y0) {
		return p->y0 < q->y0 ? -1 : 1;
	}
	return p->index < q->index ? -1 : (p->index > q->index ? 1 : 0);
}

/* by where they are halfway down, then by index: their order all through a band no two of them cross in */
static bool pq_before(const pq_active *p, const pq_active *q)
{
	return p->middle < q->middle || (p->middle == q->middle && p->index < q->index);
}

/*
 * Sorts a band's edges by pq_before. They come in the last band's order, which differs from this one by the edges
 * that started and the few that crossed, so an insertion sort does about one pass.
 */
static void pq_sort_middle(pq_active *active, uint32_t n)
{
	for (uint32_t i = 1; i < n; i++) {
		pq_active e = active[i];
		uint32_t j = i;
		while (j > 0 && pq_before(&e, &active[j - 1])) {
			active[j] = active[j - 1];
			j--;
		}
		active[j] = e;
	}
}

/* A trapezoid still open: its two edges, its top, and where its edges are at its top. */
typedef struct {
	uint32_t left, right;
	double top, lt, rt;
} pq_open;

/* The sweep: the edges, the trapezoids still open left to right, the next band's spans, and the trapezoids made. */
typedef struct {
	const pq_edge *edges;
	pq_open *open, *spans;
	uint32_t opens, count;
	uint32_t *slot;   /* by left edge: 1 + its place in open, or 0 */
	bool *carried;    /* by place in open */
	uint32_t *top, *bottom, *rank, *sealed;   /* a band's edges by place: see pq_seal */
	pq_doubles *out;
} pq_sweep;

#define PQ_UNSEALED UINT32_MAX

/* a before b just below the top (by is_top) or just above the bottom: by x there, then halfway, then index */
static bool pq_before_at(const pq_active *a, const pq_active *b, bool is_top)
{
	double x = is_top ? a->top : a->bottom, y = is_top ? b->top : b->bottom;
	if (x != y) {
		return x < y;
	}
	if (a->middle != b->middle) {
		return a->middle < b->middle;
	}
	return a->index < b->index;
}

/* the places 0..n-1 in active, sorted by pq_before_at; they start in the order halfway down, which is near it */
static void pq_sort_at(const pq_active *active, uint32_t *places, uint32_t n, bool is_top)
{
	for (uint32_t i = 0; i < n; i++) {
		places[i] = i;
	}
	for (uint32_t i = 1; i < n; i++) {
		uint32_t e = places[i], j = i;
		while (j > 0 && pq_before_at(&active[e], &active[places[j - 1]], is_top)) {
			places[j] = places[j - 1];
			j--;
		}
		places[j] = e;
	}
}

/*
 * Under non-zero, the edges of a band (sorted halfway down) whose crossings cannot change what is filled: sealed[i]
 * is the group of the edge at place i, or PQ_UNSEALED. A group is the edges between two places where the edges left
 * of them are the same at the band's top and at its bottom: they reorder only among themselves. If no order of them
 * brings the winding between two of them to 0 (the winding left of the group, plus all of their downs at least and
 * all of their ups at most), the area between them stays filled throughout, and they cross unseen.
 */
static void pq_seal(const pq_active *active, uint32_t n, pq_sweep *sw)
{
	uint32_t start = 0;
	int64_t reach = -1, left = 0, downs = 0, ups = 0;

	for (uint32_t i = 0; i < n; i++) {
		sw->sealed[i] = PQ_UNSEALED;
	}
	if (n < 2) {
		return;
	}
	pq_sort_at(active, sw->top, n, true);
	pq_sort_at(active, sw->bottom, n, false);
	for (uint32_t p = 0; p < n; p++) {
		sw->rank[sw->bottom[p]] = p;
	}
	for (uint32_t p = 0; p < n; p++) {
		uint32_t k = sw->top[p];
		int direction = active[k].direction;
		if ((int64_t) sw->rank[k] > reach) {
			reach = sw->rank[k];
		}
		if (direction < 0) {
			downs += direction;
		} else {
			ups += direction;
		}
		if (reach != (int64_t) p) {
			continue;
		}
		/* the edges at places start..p are a group */
		if (p > start && (left + downs > 0 || left + ups < 0)) {
			for (uint32_t q = start; q <= p; q++) {
				sw->sealed[sw->top[q]] = start;
			}
		}
		left += downs + ups;
		start = p + 1;
		downs = ups = 0;
	}
}

/* Where e is at height y. */
static double pq_at(const pq_edge *e, double y)
{
	return e->x0 + (y - e->y0) * ((e->x1 - e->x0) / (e->y1 - e->y0));
}

/*
 * Settles the open trapezoids at height y against the next band's spans, left to right: one bounded by the same two
 * edges carries on, the rest end at y, and the new ones open there.
 */
static bool pq_close(pq_sweep *sw, double y, uint32_t spans)
{
	for (uint32_t k = 0; k < sw->opens; k++) {
		sw->carried[k] = false;
	}
	for (uint32_t k = 0; k < spans; k++) {
		uint32_t at = sw->slot[sw->spans[k].left];
		if (at != 0 && sw->open[at - 1].right == sw->spans[k].right) {
			sw->carried[at - 1] = true;
			sw->spans[k].top = sw->open[at - 1].top;
			sw->spans[k].lt = sw->open[at - 1].lt;
			sw->spans[k].rt = sw->open[at - 1].rt;
		}
	}
	for (uint32_t k = 0; k < sw->opens; k++) {
		const pq_open *o = &sw->open[k];
		sw->slot[o->left] = 0;
		if (!sw->carried[k]) {
			double trap[6] = { o->top, y, o->lt, pq_at(&sw->edges[o->left], y), o->rt, pq_at(&sw->edges[o->right], y) };
			if (!pq_doubles_add(sw->out, trap, 6)) {
				return false;
			}
		}
	}
	for (uint32_t k = 0; k < spans; k++) {
		sw->open[k] = sw->spans[k];
		sw->slot[sw->spans[k].left] = k + 1;
	}
	sw->opens = spans;
	return true;
}

static bool pq_band(const pq_edge *edges, uint32_t *live, uint32_t lives, double top, double bottom, bool even_odd,
	pq_active *active, pq_sweep *sw)
{
	double middle = top + (bottom - top) / 2, cut = 0.0;
	uint32_t n = 0;
	bool cutting = false;

	for (uint32_t k = 0; k < lives; k++) {
		const pq_edge *e = &edges[live[k]];
		if (e->y0 <= top && e->y1 >= bottom) {
			double slope = (e->x1 - e->x0) / (e->y1 - e->y0);
			active[n].top = e->x0 + (top - e->y0) * slope;
			active[n].bottom = e->x0 + (bottom - e->y0) * slope;
			active[n].middle = e->x0 + (middle - e->y0) * slope;
			active[n].index = live[k];
			active[n].direction = e->direction;
			n++;
		}
	}
	pq_sort_middle(active, n);
	/* the next band, and a sub-band cut from this one, start from this order (every live edge spans the band, as
	 * every edge starts and ends at a band's edge; were one not to, live keeps its order) */
	if (n == lives) {
		for (uint32_t i = 0; i < n; i++) {
			live[i] = active[i].index;
		}
	}

	if (!even_odd) {
		pq_seal(active, n, sw);
	}
	for (uint32_t i = 0; i + 1 < n; i++) {
		double lt = active[i].top, lb = active[i].bottom, rt = active[i + 1].top, rb = active[i + 1].bottom;
		if (!even_odd && sw->sealed[i] != PQ_UNSEALED && sw->sealed[i] == sw->sealed[i + 1]) {
			continue;
		}
		if (lt > rt || lb > rb) {
			double apart = (rt - lt) - (rb - lb), y;
			if (apart == 0.0) {
				continue;
			}
			y = top + (rt - lt) / apart * (bottom - top);
			if (y - top > PQ_THINNEST && bottom - y > PQ_THINNEST && (!cutting || y < cut)) {
				cut = y;
				cutting = true;
			}
		}
	}
	if (cutting) {
		/* the sub-bands sort the same edges again: active is theirs to reuse */
		return pq_band(edges, live, lives, top, cut, even_odd, active, sw)
			&& pq_band(edges, live, lives, cut, bottom, even_odd, active, sw);
	}

	int winding = 0;
	uint32_t spans = 0;
	const pq_active *left = NULL;
	for (uint32_t i = 0; i < n; i++) {
		bool inside = even_odd ? (winding & 1) == 1 : winding != 0, now;
		winding += active[i].direction;
		now = even_odd ? (winding & 1) == 1 : winding != 0;
		if (!inside && now) {
			left = &active[i];
		} else if (inside && !now) {
			sw->spans[spans++] = (pq_open) { left->index, active[i].index, top, left->top, active[i].top };
		}
	}
	return pq_close(sw, top, spans);
}

bool pq_lower_trapezoids(const pq_shape *s, bool even_odd, pq_doubles *out)
{
	pq_edge *edges;
	double *heights;
	pq_start *starts;
	uint32_t *live;
	pq_active *active;
	pq_sweep sw = { 0 };
	uint32_t count = 0, at = 0, lives = 0, next = 0;
	bool ok = true;

	edges = malloc((size_t) (s->points ? s->points : 1) * sizeof(pq_edge));
	heights = malloc((size_t) (s->points ? s->points : 1) * 2 * sizeof(double));
	if (edges == NULL || heights == NULL) {
		free(edges);
		free(heights);
		return false;
	}
	for (uint32_t c = 0; c < s->contours; c++) {
		uint32_t n = s->counts[c];
		const double *p = s->xy + 2 * at;
		for (uint32_t i = 0; i < n; i++) {
			double ax = p[2 * i], ay = p[2 * i + 1], bx = p[2 * ((i + 1) % n)], by = p[2 * ((i + 1) % n) + 1];
			if (ay == by) {
				continue;
			}
			if (ay < by) {
				edges[count] = (pq_edge) { ax, ay, bx, by, 1 };
			} else {
				edges[count] = (pq_edge) { bx, by, ax, ay, -1 };
			}
			heights[2 * count] = ay;
			heights[2 * count + 1] = by;
			count++;
		}
		at += n;
	}
	if (count == 0) {
		free(edges);
		free(heights);
		return true;
	}

	qsort(heights, (size_t) count * 2, sizeof(double), pq_by_height);
	starts = malloc((size_t) count * sizeof(pq_start));
	live = malloc((size_t) count * sizeof(uint32_t));
	active = malloc((size_t) count * sizeof(pq_active));
	sw.edges = edges;
	sw.out = out;
	sw.open = malloc((size_t) count * sizeof(pq_open));
	sw.spans = malloc((size_t) count * sizeof(pq_open));
	sw.slot = calloc(count, sizeof(uint32_t));
	sw.carried = malloc((size_t) count * sizeof(bool));
	sw.top = malloc((size_t) count * sizeof(uint32_t));
	sw.bottom = malloc((size_t) count * sizeof(uint32_t));
	sw.rank = malloc((size_t) count * sizeof(uint32_t));
	sw.sealed = malloc((size_t) count * sizeof(uint32_t));
	if (starts == NULL || live == NULL || active == NULL || sw.open == NULL || sw.spans == NULL || sw.slot == NULL
		|| sw.carried == NULL || sw.top == NULL || sw.bottom == NULL || sw.rank == NULL || sw.sealed == NULL) {
		ok = false;
		goto done;
	}
	for (uint32_t i = 0; i < count; i++) {
		starts[i] = (pq_start) { edges[i].y0, i };
	}
	qsort(starts, count, sizeof(pq_start), pq_by_start);

	for (uint32_t h = 1; h < count * 2 && ok; h++) {
		double previous = heights[h - 1], y = heights[h];
		if (!(y > previous)) {
			continue;
		}
		while (next < count && starts[next].y0 <= previous) {
			live[lives++] = starts[next++].index;
		}
		uint32_t kept = 0;
		for (uint32_t k = 0; k < lives; k++) {
			if (!(edges[live[k]].y1 <= previous)) {
				live[kept++] = live[k];
			}
		}
		lives = kept;
		ok = pq_band(edges, live, lives, previous, y, even_odd, active, &sw);
	}
	if (ok) {
		ok = pq_close(&sw, heights[count * 2 - 1], 0);
	}

done:
	free(edges);
	free(heights);
	free(starts);
	free(live);
	free(active);
	free(sw.open);
	free(sw.spans);
	free(sw.slot);
	free(sw.carried);
	free(sw.top);
	free(sw.bottom);
	free(sw.rank);
	free(sw.sealed);
	return ok;
}

bool pq_lower_triangles(const double *traps, size_t count, pq_doubles *out)
{
	for (size_t i = 0; i < count; i++) {
		const double *t = traps + 6 * i;
		double top = t[0], bottom = t[1], lt = t[2], lb = t[3], rt = t[4], rb = t[5];
		if (rt != lt) {
			double tri[6] = { lt, top, rt, top, rb, bottom };
			if (!pq_doubles_add(out, tri, 6)) {
				return false;
			}
		}
		if (rb != lb) {
			double tri[6] = { lt, top, rb, bottom, lb, bottom };
			if (!pq_doubles_add(out, tri, 6)) {
				return false;
			}
		}
	}
	return true;
}
