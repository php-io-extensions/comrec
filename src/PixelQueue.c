/*
 * PixelQueue: a frame of drawing commands recorded under a transform and a
 * clip, and compared with the frame before it.
 */

#include "runtime.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "../stubs/PixelQueue_arginfo.h"

#include <math.h>

zend_class_entry *phppq_ce_PixelQueue;

static zend_object_handlers phppq_queue_handlers;

/* The kinds the queue records itself. Made once as the module starts, and never changed. */
static zend_string *phppq_kind_clear, *phppq_kind_path, *phppq_kind_polyline, *phppq_kind_ellipse,
	*phppq_kind_ring, *phppq_kind_roundrect, *phppq_kind_roundring, *phppq_kind_shadow, *phppq_kind_glyphs;

/* The most points one flattened outline has: four corners of 257 points. */
#define PQ_OUTLINE_POINTS 1028

/* ------------------------------------------------------------------ frames */

static void phppq_frame_drop(pq_frame *f)
{
	if (f->list != NULL) {
		zend_array_release(f->list);
	}
	if (f->meta != NULL) {
		efree(f->meta);
	}
	if (f->extra != NULL) {
		efree(f->extra);
	}
	memset(f, 0, sizeof(*f));
}

static void phppq_frame_start(pq_frame *f)
{
	memset(f, 0, sizeof(*f));
	f->list = zend_new_array(0);
}

/* Put command last in the frame; the frame owns it from here. The meta comes back cleared. */
static pq_meta *phppq_frame_add(pq_frame *f, zval *command)
{
	pq_meta *m;

	zend_hash_next_index_insert(f->list, command);
	if (f->n == f->cap) {
		f->cap = f->cap == 0 ? 32 : f->cap * 2;
		f->meta = safe_erealloc(f->meta, f->cap, sizeof(pq_meta), 0);
	}
	m = &f->meta[f->n++];
	memset(m, 0, sizeof(*m));
	m->extra = PQ_NO_EXTRA;
	return m;
}

static pq_extra *phppq_frame_extra(pq_frame *f, pq_meta *m)
{
	pq_extra *e;

	if (f->nextra == f->capextra) {
		f->capextra = f->capextra == 0 ? 8 : f->capextra * 2;
		f->extra = safe_erealloc(f->extra, f->capextra, sizeof(pq_extra), 0);
	}
	m->extra = f->nextra;
	e = &f->extra[f->nextra++];
	memset(e, 0, sizeof(*e));
	return e;
}

/* ------------------------------------------------------------------ object */

static zend_object *phppq_queue_create(zend_class_entry *ce)
{
	phppq_queue *q = zend_object_alloc(sizeof(phppq_queue), ce);

	zend_object_std_init(&q->std, ce);
	object_properties_init(&q->std, ce);
	q->std.handlers = &phppq_queue_handlers;
	ZVAL_UNDEF(&q->clip_value);
	ZVAL_LONG(&q->non_zero, 0);
	ZVAL_LONG(&q->even_odd, 1);
	return &q->std;
}

/* Forget the pushes from `from` up. */
static void phppq_stack_cut(phppq_queue *q, uint32_t from)
{
	while (q->top > from) {
		zval_ptr_dtor(&q->stack[--q->top].clip_value);
	}
}

/* Drop the open frame, the layers around it and every push. */
static void phppq_close(phppq_queue *q)
{
	phppq_frame_drop(&q->open);
	while (q->nlayers > 0) {
		pq_layer *l = &q->layers[--q->nlayers];

		phppq_frame_drop(&l->frame);
		zval_ptr_dtor(&l->clip_value);
	}
	phppq_stack_cut(q, 0);
	q->base = 0;
	zval_ptr_dtor(&q->clip_value);
	ZVAL_UNDEF(&q->clip_value);
	q->recording = false;
}

static void phppq_queue_free(zend_object *object)
{
	phppq_queue *q = phppq_queue_from(object);

	phppq_close(q);
	phppq_frame_drop(&q->last);
	phppq_frame_drop(&q->before);
	if (q->stack != NULL) {
		efree(q->stack);
	}
	if (q->layers != NULL) {
		efree(q->layers);
	}
	zval_ptr_dtor(&q->non_zero);
	zval_ptr_dtor(&q->even_odd);
	zend_object_std_dtor(object);
}

static void phppq_gc_frame(zend_get_gc_buffer *buffer, const pq_frame *f)
{
	if (f->list != NULL) {
		zval list;

		ZVAL_ARR(&list, f->list);
		zend_get_gc_buffer_add_zval(buffer, &list);
	}
}

/* Everything the queue holds that PHP made: the commands may hold objects that reach back to it. */
static HashTable *phppq_queue_gc(zend_object *object, zval **table, int *count)
{
	phppq_queue *q = phppq_queue_from(object);
	zend_get_gc_buffer *buffer = zend_get_gc_buffer_create();

	phppq_gc_frame(buffer, &q->open);
	phppq_gc_frame(buffer, &q->last);
	phppq_gc_frame(buffer, &q->before);
	zend_get_gc_buffer_add_zval(buffer, &q->clip_value);
	zend_get_gc_buffer_add_zval(buffer, &q->non_zero);
	zend_get_gc_buffer_add_zval(buffer, &q->even_odd);
	for (uint32_t i = 0; i < q->top; i++) {
		zend_get_gc_buffer_add_zval(buffer, &q->stack[i].clip_value);
	}
	for (uint32_t i = 0; i < q->nlayers; i++) {
		phppq_gc_frame(buffer, &q->layers[i].frame);
		zend_get_gc_buffer_add_zval(buffer, &q->layers[i].clip_value);
	}
	zend_get_gc_buffer_use(buffer, table, count);
	return zend_std_get_properties(object);
}

void phppq_register_PixelQueue(void)
{
	phppq_ce_PixelQueue = register_class_PixelQueue();
	phppq_ce_PixelQueue->create_object = phppq_queue_create;

	memcpy(&phppq_queue_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	phppq_queue_handlers.offset = XtOffsetOf(phppq_queue, std);
	phppq_queue_handlers.free_obj = phppq_queue_free;
	phppq_queue_handlers.get_gc = phppq_queue_gc;
	phppq_queue_handlers.clone_obj = NULL;

	phppq_kind_clear = zend_string_init_interned("clear", sizeof("clear") - 1, true);
	phppq_kind_path = zend_string_init_interned("path", sizeof("path") - 1, true);
	phppq_kind_polyline = zend_string_init_interned("polyline", sizeof("polyline") - 1, true);
	phppq_kind_ellipse = zend_string_init_interned("ellipse", sizeof("ellipse") - 1, true);
	phppq_kind_ring = zend_string_init_interned("ring", sizeof("ring") - 1, true);
	phppq_kind_roundrect = zend_string_init_interned("roundrect", sizeof("roundrect") - 1, true);
	phppq_kind_roundring = zend_string_init_interned("roundring", sizeof("roundring") - 1, true);
	phppq_kind_shadow = zend_string_init_interned("shadow", sizeof("shadow") - 1, true);
	phppq_kind_glyphs = zend_string_init_interned("glyphs", sizeof("glyphs") - 1, true);
}

#define PHPPQ_THIS() phppq_queue_from(Z_OBJ_P(ZEND_THIS))

/* ---------------------------------------------------------------- refusals */

static void phppq_refuse(zend_long code, const char *what, double value)
{
	zend_object *e;

	switch (code) {
		case PQ_NOT_RECORDING:
			e = zend_throw_exception_ex(phppq_ce_PixelQueueException, code, "%s() needs an open frame: call begin() first.", what);
			break;
		case PQ_NOT_FINITE:
			e = zend_throw_exception_ex(phppq_ce_PixelQueueException, code, "%s is not a finite number.", what);
			break;
		case PQ_PAST_LIMIT:
			e = zend_throw_exception_ex(phppq_ce_PixelQueueException, code, "%s %g is past 16777216 once transformed.", what, value);
			break;
		case PQ_BAD_RADII:
			e = zend_throw_exception_ex(phppq_ce_PixelQueueException, code, "Radii are a list of four numbers: [tl, tr, br, bl].");
			break;
		case PQ_UNEVEN:
			e = zend_throw_exception_ex(phppq_ce_PixelQueueException, code, "%s() needs an upright transform that scales x and y alike.", what);
			break;
		default:
			e = zend_throw_exception_ex(phppq_ce_PixelQueueException, code, "The transform would have a component that is not finite: %g.", value);
			break;
	}
	zend_update_property_string(phppq_ce_PixelQueueException, e, "what", sizeof("what") - 1, what);
	zend_update_property_double(phppq_ce_PixelQueueException, e, "value", sizeof("value") - 1, value);
}

static zend_always_inline bool phppq_open(const phppq_queue *q, const char *call)
{
	if (!q->recording) {
		phppq_refuse(PQ_NOT_RECORDING, call, 0.0);
		return false;
	}
	return true;
}

static zend_always_inline bool phppq_finite(double value, const char *name)
{
	if (!isfinite(value)) {
		phppq_refuse(PQ_NOT_FINITE, name, 0.0);
		return false;
	}
	return true;
}

/* -------------------------------------------------------------- arithmetic */

/* (x, y) through the transform into out; each coordinate is held to the limit, x first. */
static zend_always_inline bool phppq_point(const phppq_queue *q, double x, double y, const char *x_name, const char *y_name, double *out)
{
	const double *m = q->m;
	double tx = m[0] * x + m[2] * y + m[4];
	double ty = m[1] * x + m[3] * y + m[5];

	if (!isfinite(tx) || fabs(tx) > PQ_LIMIT) {
		phppq_refuse(PQ_PAST_LIMIT, x_name, tx);
		return false;
	}
	if (!isfinite(ty) || fabs(ty) > PQ_LIMIT) {
		phppq_refuse(PQ_PAST_LIMIT, y_name, ty);
		return false;
	}
	out[0] = tx;
	out[1] = ty;
	return true;
}

/* A rect's corners through the transform: top-left, top-right, bottom-right, bottom-left. */
static bool phppq_corners(const phppq_queue *q, double x, double y, double w, double h, double *out)
{
	return phppq_point(q, x, y, "x", "y", out)
		&& phppq_point(q, x + w, y, "x", "y", out + 2)
		&& phppq_point(q, x + w, y + h, "x", "y", out + 4)
		&& phppq_point(q, x, y + h, "x", "y", out + 6);
}

/* A stroke as wide as the transform makes it: scaled by the square root of the area it scales by. */
static bool phppq_stroke_width(const phppq_queue *q, double stroke, double *out)
{
	const double *m = q->m;
	double scaled = stroke * sqrt(fabs(m[0] * m[3] - m[1] * m[2]));

	if (fabs(scaled) > PQ_LIMIT) {
		phppq_refuse(PQ_PAST_LIMIT, "stroke", scaled);
		return false;
	}
	*out = scaled;
	return true;
}

/* No turn and no shear. */
static zend_always_inline bool phppq_aligned(const phppq_queue *q)
{
	return q->m[1] == 0.0 && q->m[2] == 0.0;
}

/* Upright, x and y scaled alike, by more than 0. */
static zend_always_inline bool phppq_even(const phppq_queue *q)
{
	return q->m[1] == 0.0 && q->m[2] == 0.0 && q->m[0] == q->m[3] && q->m[0] > 0.0;
}

/* The longest a unit length becomes under the transform. */
static zend_always_inline double phppq_stretch(const phppq_queue *q)
{
	const double *m = q->m;
	double along = m[0] * m[0] + m[1] * m[1];
	double across = m[2] * m[2] + m[3] * m[3];

	return sqrt(along < across ? across : along);
}

/* How many straight sides keep a circle of this radius, in pixels, within PQ_FLATNESS: 12..1024. */
static zend_always_inline zend_long phppq_sides(double radius)
{
	double sides = ceil(M_PI * sqrt(radius / (2 * PQ_FLATNESS)) * 1.01);

	if (!(sides < 1024.0)) {
		sides = 1024.0;
	}
	if (!(sides > 12.0)) {
		sides = 12.0;
	}
	return (zend_long) sides;
}

/* An ellipse as a polygon through the transform; the count of its points, or 0 with a refusal thrown. */
static uint32_t phppq_flatten(const phppq_queue *q, double cx, double cy, double rx, double ry, double *out)
{
	zend_long sides = phppq_sides((rx < ry ? ry : rx) * phppq_stretch(q));

	for (zend_long i = 0; i < sides; i++) {
		double angle = 2 * M_PI * (double) i / (double) sides;

		if (!phppq_point(q, cx + rx * cos(angle), cy + ry * sin(angle), "cx", "cy", out + 2 * i)) {
			return 0;
		}
	}
	return (uint32_t) sides;
}

/*
 * A rounded rect's outline through the transform, clockwise from the top-left
 * corner; the count of its points, or 0 with a refusal thrown.
 */
static uint32_t phppq_outline(const phppq_queue *q, double x, double y, double w, double h, const double radii[4], double *out)
{
	double stretch = phppq_stretch(q);
	double corner[4][4] = {
		{x + radii[0], y + radii[0], radii[0], M_PI},
		{x + w - radii[1], y + radii[1], radii[1], 1.5 * M_PI},
		{x + w - radii[2], y + h - radii[2], radii[2], 0.0},
		{x + radii[3], y + h - radii[3], radii[3], 0.5 * M_PI},
	};
	uint32_t n = 0;

	for (int k = 0; k < 4; k++) {
		double cx = corner[k][0], cy = corner[k][1], r = corner[k][2], from = corner[k][3];
		zend_long steps;

		if (r <= 0) {
			if (!phppq_point(q, cx, cy, "x", "y", out + 2 * n)) {
				return 0;
			}
			n++;
			continue;
		}
		steps = (phppq_sides(r * stretch) + 3) / 4;
		for (zend_long i = 0; i <= steps; i++) {
			double angle = from + 0.5 * M_PI * (double) i / (double) steps;

			if (!phppq_point(q, cx + r * cos(angle), cy + r * sin(angle), "x", "y", out + 2 * n)) {
				return 0;
			}
			n++;
		}
	}
	return n;
}

/* ---------------------------------------------------------------- commands */

static zend_always_inline bool phppq_clip_empty(const phppq_queue *q)
{
	return q->clip.w <= 0 || q->clip.h <= 0;
}

static zend_always_inline void phppq_add_double(zend_array *list, double value)
{
	zval z;

	ZVAL_DOUBLE(&z, value);
	zend_hash_next_index_insert_new(list, &z);
}

static zend_always_inline void phppq_add_long(zend_array *list, zend_long value)
{
	zval z;

	ZVAL_LONG(&z, value);
	zend_hash_next_index_insert_new(list, &z);
}

static zend_always_inline void phppq_add_kind(zend_array *list, zend_string *kind)
{
	zval z;

	ZVAL_INTERNED_STR(&z, kind);
	zend_hash_next_index_insert_new(list, &z);
}

static zend_always_inline void phppq_add_copy(zend_array *list, zval *value)
{
	zval z;

	ZVAL_COPY(&z, value);
	zend_hash_next_index_insert_new(list, &z);
}

/* A list of [x, y] from n points. */
static void phppq_points(zval *out, const double *points, uint32_t n)
{
	zend_array *list = zend_new_array(n);

	for (uint32_t i = 0; i < n; i++) {
		zval x, y, pair;

		ZVAL_DOUBLE(&x, points[2 * i]);
		ZVAL_DOUBLE(&y, points[2 * i + 1]);
		ZVAL_ARR(&pair, zend_new_pair(&x, &y));
		zend_hash_next_index_insert_new(list, &pair);
	}
	ZVAL_ARR(out, list);
}

/* The smallest and largest x and y of n points, n at least 1, joined into reach when it is already set. */
static void phppq_reach(const double *points, uint32_t n, double reach[4], bool set)
{
	uint32_t i = 0;

	if (!set) {
		reach[0] = reach[2] = points[0];
		reach[1] = reach[3] = points[1];
		i = 1;
	}
	for (; i < n; i++) {
		double x = points[2 * i], y = points[2 * i + 1];

		if (x < reach[0]) {
			reach[0] = x;
		}
		if (x > reach[2]) {
			reach[2] = x;
		}
		if (y < reach[1]) {
			reach[1] = y;
		}
		if (y > reach[3]) {
			reach[3] = y;
		}
	}
}

static zend_always_inline void phppq_meta_reach(phppq_queue *q, pq_meta *m, double x0, double y0, double x1, double y1)
{
	m->how = PQ_HOW_ONE;
	m->piece.x0 = x0;
	m->piece.y0 = y0;
	m->piece.x1 = x1;
	m->piece.y1 = y1;
	m->piece.clip = q->clip;
}

/* ['path', contours, rule, rgba, clip] of one or two contours; nothing under an empty clip. */
static void phppq_record_path(phppq_queue *q, const double *first, uint32_t first_n, const double *second, uint32_t second_n, zval *rule, zend_long rgba)
{
	zval command, contours, contour;
	zend_array *list;
	double reach[4];

	if (phppq_clip_empty(q)) {
		return;
	}
	array_init_size(&contours, second == NULL ? 1 : 2);
	phppq_points(&contour, first, first_n);
	zend_hash_next_index_insert_new(Z_ARRVAL(contours), &contour);
	phppq_reach(first, first_n, reach, false);
	if (second != NULL) {
		phppq_points(&contour, second, second_n);
		zend_hash_next_index_insert_new(Z_ARRVAL(contours), &contour);
		phppq_reach(second, second_n, reach, true);
	}

	list = zend_new_array(5);
	phppq_add_kind(list, phppq_kind_path);
	zend_hash_next_index_insert_new(list, &contours);
	phppq_add_copy(list, rule);
	phppq_add_long(list, rgba);
	phppq_add_copy(list, &q->clip_value);
	ZVAL_ARR(&command, list);
	phppq_meta_reach(q, phppq_frame_add(&q->open, &command), reach[0] - 1.0, reach[1] - 1.0, reach[2] + 1.0, reach[3] + 1.0);
}

/* ['polyline', points, stroke, closed, rgba, clip] with the stroke scaled; false with a refusal thrown. */
static bool phppq_record_stroked(phppq_queue *q, const double *points, uint32_t n, double stroke, bool closed, zend_long rgba)
{
	zval command, list_of_points, flag;
	zend_array *list;
	double reach[4], grow;

	if (!phppq_stroke_width(q, stroke, &stroke)) {
		return false;
	}
	if (!(stroke > 0) || phppq_clip_empty(q)) {
		return true;
	}
	phppq_points(&list_of_points, points, n);
	phppq_reach(points, n, reach, false);

	list = zend_new_array(6);
	phppq_add_kind(list, phppq_kind_polyline);
	zend_hash_next_index_insert_new(list, &list_of_points);
	phppq_add_double(list, stroke);
	ZVAL_BOOL(&flag, closed);
	zend_hash_next_index_insert_new(list, &flag);
	phppq_add_long(list, rgba);
	phppq_add_copy(list, &q->clip_value);
	ZVAL_ARR(&command, list);
	grow = 2.0 * stroke + 1.0;
	phppq_meta_reach(q, phppq_frame_add(&q->open, &command), reach[0] - grow, reach[1] - grow, reach[2] + grow, reach[3] + grow);
	return true;
}

/* [kind, x0, y0, x1, y1, [tl, tr, br, bl], (inset,) rgba, clip]: a 'roundrect', or a 'roundring' when inset is given. */
static void phppq_record_round(phppq_queue *q, zend_string *kind, double x0, double y0, double x1, double y1, const double radii[4], double scale, const double *inset, zend_long rgba)
{
	zval command, corners;
	zend_array *list = zend_new_array(8), *four = zend_new_array(4);

	for (int i = 0; i < 4; i++) {
		phppq_add_double(four, radii[i] * scale);
	}
	ZVAL_ARR(&corners, four);

	phppq_add_kind(list, kind);
	phppq_add_double(list, x0);
	phppq_add_double(list, y0);
	phppq_add_double(list, x1);
	phppq_add_double(list, y1);
	zend_hash_next_index_insert_new(list, &corners);
	if (inset != NULL) {
		phppq_add_double(list, *inset);
	}
	phppq_add_long(list, rgba);
	phppq_add_copy(list, &q->clip_value);
	ZVAL_ARR(&command, list);
	phppq_meta_reach(q, phppq_frame_add(&q->open, &command), x0 - 1.0, y0 - 1.0, x1 + 1.0, y1 + 1.0);
}

/* ------------------------------------------------------------------- frame */

PHP_METHOD(PixelQueue, __construct)
{
	zval *non_zero = NULL, *even_odd = NULL;
	phppq_queue *q;

	(void) return_value;
	ZEND_PARSE_PARAMETERS_START(0, 2)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(non_zero)
		Z_PARAM_ZVAL(even_odd)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (non_zero != NULL) {
		zval_ptr_dtor(&q->non_zero);
		ZVAL_COPY(&q->non_zero, non_zero);
	}
	if (even_odd != NULL) {
		zval_ptr_dtor(&q->even_odd);
		ZVAL_COPY(&q->even_odd, even_odd);
	}
}

/* Whether x, y, width, height is a rect the queue can hold: every edge within the limit. */
static bool phppq_rect(zend_long x, zend_long y, zend_long width, zend_long height, pq_rect *out, uint32_t first_arg)
{
	const zend_long limit = (zend_long) PQ_LIMIT;
	const zend_long value[4] = {x, y, width, height};

	for (uint32_t i = 0; i < 4; i++) {
		if (value[i] < -limit || value[i] > limit) {
			zend_argument_value_error(first_arg + i, "must be between -%d and %d", (int) limit, (int) limit);
			return false;
		}
	}
	out->x = x;
	out->y = y;
	out->w = width;
	out->h = height;
	return true;
}

PHP_METHOD(PixelQueue, begin)
{
	zend_long width, height;
	zval *clip;
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
		Z_PARAM_ZVAL(clip)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (q->recording) {
		zend_throw_error(NULL, "A frame is already open: finish() or abandon() it before begin()");
		RETURN_THROWS();
	}
	if (width < 1 || width > (zend_long) PQ_LIMIT) {
		zend_argument_value_error(1, "must be between 1 and %d", (int) PQ_LIMIT);
		RETURN_THROWS();
	}
	if (height < 1 || height > (zend_long) PQ_LIMIT) {
		zend_argument_value_error(2, "must be between 1 and %d", (int) PQ_LIMIT);
		RETURN_THROWS();
	}

	phppq_frame_start(&q->open);
	q->m[0] = 1.0;
	q->m[1] = 0.0;
	q->m[2] = 0.0;
	q->m[3] = 1.0;
	q->m[4] = 0.0;
	q->m[5] = 0.0;
	ZVAL_COPY(&q->clip_value, clip);
	q->clip.x = 0;
	q->clip.y = 0;
	q->clip.w = width;
	q->clip.h = height;
	q->recording = true;
}

PHP_METHOD(PixelQueue, recording)
{
	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(PHPPQ_THIS()->recording);
}

PHP_METHOD(PixelQueue, abandon)
{
	(void) return_value;
	ZEND_PARSE_PARAMETERS_NONE();

	phppq_close(PHPPQ_THIS());
}

PHP_METHOD(PixelQueue, finish)
{
	phppq_queue *q;
	pq_frame frame;

	ZEND_PARSE_PARAMETERS_NONE();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}
	/* A layer still entered is not part of the frame: its children are dropped with it. */
	while (q->nlayers > 0) {
		pq_layer *l = &q->layers[--q->nlayers];

		phppq_frame_drop(&q->open);
		q->open = l->frame;
		zval_ptr_dtor(&l->clip_value);
	}
	frame = q->open;
	memset(&q->open, 0, sizeof(q->open));
	phppq_close(q);

	phppq_frame_drop(&q->before);
	q->before = q->last;
	q->last = frame;

	GC_ADDREF(q->last.list);
	RETURN_ARR(q->last.list);
}

/* ------------------------------------------------------------------- state */

PHP_METHOD(PixelQueue, push)
{
	phppq_queue *q;
	pq_saved *s;

	(void) return_value;
	ZEND_PARSE_PARAMETERS_NONE();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}
	if (q->top == q->stack_cap) {
		q->stack_cap = q->stack_cap == 0 ? 8 : q->stack_cap * 2;
		q->stack = safe_erealloc(q->stack, q->stack_cap, sizeof(pq_saved), 0);
	}
	s = &q->stack[q->top++];
	memcpy(s->m, q->m, sizeof(q->m));
	ZVAL_COPY(&s->clip_value, &q->clip_value);
	s->clip = q->clip;
}

PHP_METHOD(PixelQueue, pop)
{
	phppq_queue *q;
	pq_saved *s;

	ZEND_PARSE_PARAMETERS_NONE();

	q = PHPPQ_THIS();
	if (!q->recording || q->top == q->base) {
		zend_throw_error(NULL, "pop() without a push()");
		RETURN_THROWS();
	}
	s = &q->stack[--q->top];
	memcpy(q->m, s->m, sizeof(q->m));
	zval_ptr_dtor(&q->clip_value);
	ZVAL_COPY_VALUE(&q->clip_value, &s->clip_value);
	q->clip = s->clip;
}

PHP_METHOD(PixelQueue, depth)
{
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_NONE();

	q = PHPPQ_THIS();
	RETURN_LONG((zend_long) (q->top - q->base));
}

PHP_METHOD(PixelQueue, concat)
{
	double a, b, c, d, e, f, next[6];
	phppq_queue *q;
	const double *m;

	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_DOUBLE(a)
		Z_PARAM_DOUBLE(b)
		Z_PARAM_DOUBLE(c)
		Z_PARAM_DOUBLE(d)
		Z_PARAM_DOUBLE(e)
		Z_PARAM_DOUBLE(f)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}
	m = q->m;
	next[0] = m[0] * a + m[2] * b;
	next[1] = m[1] * a + m[3] * b;
	next[2] = m[0] * c + m[2] * d;
	next[3] = m[1] * c + m[3] * d;
	next[4] = m[0] * e + m[2] * f + m[4];
	next[5] = m[1] * e + m[3] * f + m[5];
	for (int i = 0; i < 6; i++) {
		if (!isfinite(next[i])) {
			phppq_refuse(PQ_BAD_MATRIX, "", next[i]);
			RETURN_THROWS();
		}
	}
	memcpy(q->m, next, sizeof(next));
}

PHP_METHOD(PixelQueue, matrix)
{
	phppq_queue *q;
	zend_array *list;

	ZEND_PARSE_PARAMETERS_NONE();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}
	list = zend_new_array(6);
	for (int i = 0; i < 6; i++) {
		phppq_add_double(list, q->m[i]);
	}
	RETURN_ARR(list);
}

PHP_METHOD(PixelQueue, clip)
{
	zval *clip;
	zend_long x, y, width, height;
	phppq_queue *q;
	pq_rect rect;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_ZVAL(clip)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}
	if (!phppq_rect(x, y, width, height, &rect, 2)) {
		RETURN_THROWS();
	}
	Z_TRY_ADDREF_P(clip);
	zval_ptr_dtor(&q->clip_value);
	ZVAL_COPY_VALUE(&q->clip_value, clip);
	q->clip = rect;
}

PHP_METHOD(PixelQueue, clipValue)
{
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_NONE();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}
	RETURN_COPY(&q->clip_value);
}

/* ------------------------------------------------------------------ append */

/* count numbers of a list into out; false, with a ValueError thrown, when it is not that. */
static bool phppq_numbers(HashTable *list, uint32_t count, double *out, uint32_t arg, const char *what)
{
	zval *value;
	uint32_t i = 0;

	if (!zend_array_is_list(list) || zend_hash_num_elements(list) != count) {
		zend_argument_value_error(arg, "must be %s", what);
		return false;
	}
	ZEND_HASH_FOREACH_VAL(list, value) {
		ZVAL_DEREF(value);
		if (Z_TYPE_P(value) == IS_LONG) {
			out[i] = (double) Z_LVAL_P(value);
		} else if (Z_TYPE_P(value) == IS_DOUBLE && isfinite(Z_DVAL_P(value))) {
			out[i] = Z_DVAL_P(value);
		} else {
			zend_argument_value_error(arg, "must be %s", what);
			return false;
		}
		i++;
	} ZEND_HASH_FOREACH_END();
	return true;
}

/* Four numbers as a rect whose edges are whole and within the limit. */
static bool phppq_whole_rect(const double *n, pq_rect *out)
{
	for (int i = 0; i < 4; i++) {
		if (n[i] != floor(n[i]) || fabs(n[i]) > PQ_LIMIT) {
			return false;
		}
	}
	out->x = (int64_t) n[0];
	out->y = (int64_t) n[1];
	out->w = (int64_t) n[2];
	out->h = (int64_t) n[3];
	return true;
}

PHP_METHOD(PixelQueue, append)
{
	zval *command;
	HashTable *pieces = NULL, *reads = NULL;
	zend_long how;
	double grow = 0.0;
	bool restless = false;
	phppq_queue *q;
	pq_meta meta, *m;
	pq_extra extra;
	bool has_extra = false;
	zval copy;

	ZEND_PARSE_PARAMETERS_START(2, 6)
		Z_PARAM_ARRAY(command)
		Z_PARAM_LONG(how)
		Z_PARAM_OPTIONAL
		Z_PARAM_DOUBLE(grow)
		Z_PARAM_ARRAY_HT_OR_NULL(pieces)
		Z_PARAM_BOOL(restless)
		Z_PARAM_ARRAY_HT_OR_NULL(reads)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}

	memset(&meta, 0, sizeof(meta));
	memset(&extra, 0, sizeof(extra));
	meta.extra = PQ_NO_EXTRA;
	meta.restless = restless;
	meta.piece.clip = q->clip;
	switch (how) {
		case PQ_HOW_WHOLE:
		case PQ_HOW_SPANS:
			meta.how = (uint8_t) how;
			break;
		case PQ_HOW_CONTOURS:
		case PQ_HOW_POINTS:
			if (!isfinite(grow) || grow < 0.0) {
				zend_argument_value_error(3, "must be a finite number, 0 or more");
				RETURN_THROWS();
			}
			meta.how = (uint8_t) how;
			meta.piece.x0 = grow;
			break;
		case PQ_HOW_PIECES: {
			double n[16];
			uint32_t count = pieces == NULL ? 0 : zend_hash_num_elements(pieces);

			if (count != 8 && count != 16) {
				zend_argument_value_error(4, "must be a list of 8 or 16 numbers for PIXEL_QUEUE_PIECES");
				RETURN_THROWS();
			}
			if (!phppq_numbers(pieces, count, n, 4, "a list of 8 or 16 finite numbers")) {
				RETURN_THROWS();
			}
			if (!phppq_whole_rect(n + 4, &meta.piece.clip) || (count == 16 && !phppq_whole_rect(n + 12, &extra.second.clip))) {
				zend_argument_value_error(4, "must give each clip as four whole numbers within the limit");
				RETURN_THROWS();
			}
			meta.how = PQ_HOW_PIECES;
			meta.piece.x0 = n[0];
			meta.piece.y0 = n[1];
			meta.piece.x1 = n[2];
			meta.piece.y1 = n[3];
			if (count == 16) {
				has_extra = true;
				extra.has_second = true;
				extra.second.x0 = n[8];
				extra.second.y0 = n[9];
				extra.second.x1 = n[10];
				extra.second.y1 = n[11];
			}
			break;
		}
		default:
			zend_argument_value_error(2, "must be one of PIXEL_QUEUE_WHOLE, PIXEL_QUEUE_CONTOURS, PIXEL_QUEUE_POINTS, PIXEL_QUEUE_SPANS or PIXEL_QUEUE_PIECES");
			RETURN_THROWS();
	}
	if (reads != NULL) {
		double n[5];

		if (!phppq_numbers(reads, 5, n, 6, "[x, y, width, height, reach], five finite numbers")) {
			RETURN_THROWS();
		}
		if (!phppq_whole_rect(n, &extra.area) || n[4] < 0.0 || n[4] > PQ_LIMIT) {
			zend_argument_value_error(6, "must give a whole rect within the limit and a reach of 0 or more");
			RETURN_THROWS();
		}
		has_extra = true;
		extra.reads = true;
		extra.reach = n[4];
	}

	ZVAL_COPY(&copy, command);
	m = phppq_frame_add(&q->open, &copy);
	*m = meta;
	if (has_extra) {
		*phppq_frame_extra(&q->open, m) = extra;
	}
}

/* ------------------------------------------------------------------ layers */

PHP_METHOD(PixelQueue, enter)
{
	zval *clip;
	zend_long x, y, width, height;
	phppq_queue *q;
	pq_layer *l;
	pq_rect rect;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_ZVAL(clip)
		Z_PARAM_LONG(x)
		Z_PARAM_LONG(y)
		Z_PARAM_LONG(width)
		Z_PARAM_LONG(height)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!q->recording) {
		zend_throw_error(NULL, "No frame is open: call begin() first");
		RETURN_THROWS();
	}
	if (!phppq_rect(x, y, width, height, &rect, 2)) {
		RETURN_THROWS();
	}
	if (q->nlayers == q->layers_cap) {
		q->layers_cap = q->layers_cap == 0 ? 4 : q->layers_cap * 2;
		q->layers = safe_erealloc(q->layers, q->layers_cap, sizeof(pq_layer), 0);
	}
	l = &q->layers[q->nlayers++];
	l->frame = q->open;
	ZVAL_COPY_VALUE(&l->clip_value, &q->clip_value);
	l->clip = q->clip;
	l->base = q->base;

	phppq_frame_start(&q->open);
	ZVAL_COPY(&q->clip_value, clip);
	q->clip = rect;
	q->base = q->top;
}

PHP_METHOD(PixelQueue, leave)
{
	phppq_queue *q;
	pq_layer *l;
	zend_array *children;

	ZEND_PARSE_PARAMETERS_NONE();

	q = PHPPQ_THIS();
	if (!q->recording || q->nlayers == 0) {
		zend_throw_error(NULL, "leave() without an enter()");
		RETURN_THROWS();
	}
	children = q->open.list;
	q->open.list = NULL;
	phppq_frame_drop(&q->open);

	/* the pushes the children left behind go with them */
	phppq_stack_cut(q, q->base);
	l = &q->layers[--q->nlayers];
	q->open = l->frame;
	zval_ptr_dtor(&q->clip_value);
	ZVAL_COPY_VALUE(&q->clip_value, &l->clip_value);
	q->clip = l->clip;
	q->base = l->base;

	RETURN_ARR(children);
}

/* ------------------------------------------------------------------ shapes */

PHP_METHOD(PixelQueue, clear)
{
	zend_long rgba;
	phppq_queue *q;
	zval command;
	zend_array *list;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(rgba)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "clear")) {
		RETURN_THROWS();
	}
	list = zend_new_array(2);
	phppq_add_kind(list, phppq_kind_clear);
	phppq_add_long(list, rgba);
	ZVAL_ARR(&command, list);
	phppq_frame_add(&q->open, &command)->how = PQ_HOW_WHOLE;
}

PHP_METHOD(PixelQueue, fillRect)
{
	double x, y, width, height, corners[8];
	zend_long rgba;
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_DOUBLE(x)
		Z_PARAM_DOUBLE(y)
		Z_PARAM_DOUBLE(width)
		Z_PARAM_DOUBLE(height)
		Z_PARAM_LONG(rgba)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "fillRect") || !phppq_finite(x, "x") || !phppq_finite(y, "y") || !phppq_finite(width, "width") || !phppq_finite(height, "height")) {
		RETURN_THROWS();
	}
	if (width <= 0 || height <= 0) {
		return;
	}
	if (!phppq_corners(q, x, y, width, height, corners)) {
		RETURN_THROWS();
	}
	phppq_record_path(q, corners, 4, NULL, 0, &q->non_zero, rgba);
}

PHP_METHOD(PixelQueue, strokeRect)
{
	double x, y, width, height, stroke, corners[8];
	zend_long rgba;
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_DOUBLE(x)
		Z_PARAM_DOUBLE(y)
		Z_PARAM_DOUBLE(width)
		Z_PARAM_DOUBLE(height)
		Z_PARAM_LONG(rgba)
		Z_PARAM_DOUBLE(stroke)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "strokeRect") || !phppq_finite(x, "x") || !phppq_finite(y, "y") || !phppq_finite(width, "width") || !phppq_finite(height, "height")
		|| !phppq_finite(stroke, "stroke")) {
		RETURN_THROWS();
	}
	if (width <= 0 || height <= 0) {
		return;
	}
	if (!phppq_corners(q, x, y, width, height, corners) || !phppq_record_stroked(q, corners, 4, stroke, true, rgba)) {
		RETURN_THROWS();
	}
}

PHP_METHOD(PixelQueue, line)
{
	double x0, y0, x1, y1, stroke, points[4];
	zend_long rgba;
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_DOUBLE(x0)
		Z_PARAM_DOUBLE(y0)
		Z_PARAM_DOUBLE(x1)
		Z_PARAM_DOUBLE(y1)
		Z_PARAM_LONG(rgba)
		Z_PARAM_DOUBLE(stroke)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "line") || !phppq_finite(x0, "x0") || !phppq_finite(y0, "y0") || !phppq_finite(x1, "x1") || !phppq_finite(y1, "y1")
		|| !phppq_finite(stroke, "stroke")) {
		RETURN_THROWS();
	}
	if (!phppq_point(q, x0, y0, "x0", "y0", points) || !phppq_point(q, x1, y1, "x1", "y1", points + 2)
		|| !phppq_record_stroked(q, points, 2, stroke, false, rgba)) {
		RETURN_THROWS();
	}
}

PHP_METHOD(PixelQueue, fillTriangle)
{
	double x0, y0, x1, y1, x2, y2, points[6];
	zend_long rgba;
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_START(7, 7)
		Z_PARAM_DOUBLE(x0)
		Z_PARAM_DOUBLE(y0)
		Z_PARAM_DOUBLE(x1)
		Z_PARAM_DOUBLE(y1)
		Z_PARAM_DOUBLE(x2)
		Z_PARAM_DOUBLE(y2)
		Z_PARAM_LONG(rgba)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "fillTriangle") || !phppq_finite(x0, "x0") || !phppq_finite(y0, "y0") || !phppq_finite(x1, "x1") || !phppq_finite(y1, "y1")
		|| !phppq_finite(x2, "x2") || !phppq_finite(y2, "y2")) {
		RETURN_THROWS();
	}
	if (!phppq_point(q, x0, y0, "x0", "y0", points) || !phppq_point(q, x1, y1, "x1", "y1", points + 2) || !phppq_point(q, x2, y2, "x2", "y2", points + 4)) {
		RETURN_THROWS();
	}
	phppq_record_path(q, points, 3, NULL, 0, &q->non_zero, rgba);
}

/* An ellipse under a transform that keeps its axes: its centre and radii in pixels; false with a refusal thrown. */
static bool phppq_upright(const phppq_queue *q, double cx, double cy, double rx, double ry, double out[4])
{
	if (!phppq_point(q, cx, cy, "cx", "cy", out)) {
		return false;
	}
	out[2] = rx * fabs(q->m[0]);
	out[3] = ry * fabs(q->m[3]);
	if (out[2] > PQ_LIMIT) {
		phppq_refuse(PQ_PAST_LIMIT, "rx", out[2]);
		return false;
	}
	if (out[3] > PQ_LIMIT) {
		phppq_refuse(PQ_PAST_LIMIT, "ry", out[3]);
		return false;
	}
	return true;
}

PHP_METHOD(PixelQueue, fillEllipse)
{
	double cx, cy, rx, ry, e[4];
	zend_long rgba;
	phppq_queue *q;
	zval command;
	zend_array *list;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_DOUBLE(cx)
		Z_PARAM_DOUBLE(cy)
		Z_PARAM_DOUBLE(rx)
		Z_PARAM_DOUBLE(ry)
		Z_PARAM_LONG(rgba)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "fillEllipse") || !phppq_finite(cx, "cx") || !phppq_finite(cy, "cy") || !phppq_finite(rx, "rx") || !phppq_finite(ry, "ry")) {
		RETURN_THROWS();
	}
	if (rx <= 0 || ry <= 0) {
		return;
	}
	if (!phppq_aligned(q)) {
		double points[2 * 1024];
		uint32_t n = phppq_flatten(q, cx, cy, rx, ry, points);

		if (n == 0) {
			RETURN_THROWS();
		}
		phppq_record_path(q, points, n, NULL, 0, &q->non_zero, rgba);
		return;
	}
	if (!phppq_upright(q, cx, cy, rx, ry, e)) {
		RETURN_THROWS();
	}
	if (!(e[2] > 0) || !(e[3] > 0) || phppq_clip_empty(q)) {
		return;
	}
	list = zend_new_array(7);
	phppq_add_kind(list, phppq_kind_ellipse);
	phppq_add_double(list, e[0]);
	phppq_add_double(list, e[1]);
	phppq_add_double(list, e[2]);
	phppq_add_double(list, e[3]);
	phppq_add_long(list, rgba);
	phppq_add_copy(list, &q->clip_value);
	ZVAL_ARR(&command, list);
	phppq_meta_reach(q, phppq_frame_add(&q->open, &command), e[0] - e[2] - 1.0, e[1] - e[3] - 1.0, e[0] + e[2] + 1.0, e[1] + e[3] + 1.0);
}

PHP_METHOD(PixelQueue, strokeEllipse)
{
	double cx, cy, rx, ry, stroke, e[4];
	zend_long rgba;
	phppq_queue *q;
	zval command;
	zend_array *list;

	ZEND_PARSE_PARAMETERS_START(6, 6)
		Z_PARAM_DOUBLE(cx)
		Z_PARAM_DOUBLE(cy)
		Z_PARAM_DOUBLE(rx)
		Z_PARAM_DOUBLE(ry)
		Z_PARAM_LONG(rgba)
		Z_PARAM_DOUBLE(stroke)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "strokeEllipse") || !phppq_finite(cx, "cx") || !phppq_finite(cy, "cy") || !phppq_finite(rx, "rx") || !phppq_finite(ry, "ry")
		|| !phppq_finite(stroke, "stroke")) {
		RETURN_THROWS();
	}
	if (rx <= 0 || ry <= 0) {
		return;
	}
	if (!phppq_aligned(q)) {
		double points[2 * 1024];
		uint32_t n = phppq_flatten(q, cx, cy, rx, ry, points);

		if (n == 0 || !phppq_record_stroked(q, points, n, stroke, true, rgba)) {
			RETURN_THROWS();
		}
		return;
	}
	if (!phppq_upright(q, cx, cy, rx, ry, e) || !phppq_stroke_width(q, stroke, &stroke)) {
		RETURN_THROWS();
	}
	if (!(e[2] > 0) || !(e[3] > 0) || !(stroke > 0) || phppq_clip_empty(q)) {
		return;
	}
	list = zend_new_array(8);
	phppq_add_kind(list, phppq_kind_ring);
	phppq_add_double(list, e[0]);
	phppq_add_double(list, e[1]);
	phppq_add_double(list, e[2]);
	phppq_add_double(list, e[3]);
	phppq_add_double(list, stroke);
	phppq_add_long(list, rgba);
	phppq_add_copy(list, &q->clip_value);
	ZVAL_ARR(&command, list);
	phppq_meta_reach(q, phppq_frame_add(&q->open, &command),
		e[0] - e[2] - stroke / 2 - 1.0, e[1] - e[3] - stroke / 2 - 1.0, e[0] + e[2] + stroke / 2 + 1.0, e[1] + e[3] + stroke / 2 + 1.0);
}

/* Four radii, each held to 0..min(width, height) / 2; false with a refusal thrown. */
static bool phppq_radii(HashTable *radii, double width, double height, double held[4])
{
	static const char *const name[4] = {"radius 0", "radius 1", "radius 2", "radius 3"};
	double half = (height < width ? height : width) / 2;
	zval *value;
	uint32_t i = 0;

	if (!zend_array_is_list(radii) || zend_hash_num_elements(radii) != 4) {
		phppq_refuse(PQ_BAD_RADII, "", 0.0);
		return false;
	}
	ZEND_HASH_FOREACH_VAL(radii, value) {
		double r;

		ZVAL_DEREF(value);
		if (Z_TYPE_P(value) == IS_LONG) {
			r = (double) Z_LVAL_P(value);
		} else if (Z_TYPE_P(value) == IS_DOUBLE) {
			r = Z_DVAL_P(value);
		} else {
			phppq_refuse(PQ_BAD_RADII, "", 0.0);
			return false;
		}
		if (!phppq_finite(r, name[i])) {
			return false;
		}
		if (half < r) {
			r = half;
		}
		held[i++] = r > 0.0 ? r : 0.0;
	} ZEND_HASH_FOREACH_END();
	return true;
}

PHP_METHOD(PixelQueue, roundRect)
{
	double x, y, width, height, border_width, radii[4], inner[4], bw, scale, inset, a[2], b[2];
	zend_long fill, border = 0;
	bool no_border = true, hollow;
	HashTable *given;
	phppq_queue *q;

	ZEND_PARSE_PARAMETERS_START(8, 8)
		Z_PARAM_DOUBLE(x)
		Z_PARAM_DOUBLE(y)
		Z_PARAM_DOUBLE(width)
		Z_PARAM_DOUBLE(height)
		Z_PARAM_ARRAY_HT(given)
		Z_PARAM_LONG(fill)
		Z_PARAM_LONG_OR_NULL(border, no_border)
		Z_PARAM_DOUBLE(border_width)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "roundRect") || !phppq_finite(x, "x") || !phppq_finite(y, "y") || !phppq_finite(width, "width") || !phppq_finite(height, "height")
		|| !phppq_finite(border_width, "borderWidth") || !phppq_radii(given, width, height, radii)) {
		RETURN_THROWS();
	}
	if (width <= 0 || height <= 0) {
		return;
	}
	bw = no_border ? 0.0 : (border_width > 0.0 ? border_width : 0.0);
	for (int i = 0; i < 4; i++) {
		double r = radii[i] - bw;

		inner[i] = r > 0.0 ? r : 0.0;
	}
	hollow = width > 2 * bw && height > 2 * bw;

	if (!phppq_even(q)) {
		/* A turned rounded rect is a path of straight sides, as a turned ellipse is. */
		double outer[2 * PQ_OUTLINE_POINTS], hole[2 * PQ_OUTLINE_POINTS];
		uint32_t outer_n = phppq_outline(q, x, y, width, height, radii, outer), hole_n = 0;

		if (outer_n == 0) {
			RETURN_THROWS();
		}
		if (bw <= 0) {
			phppq_record_path(q, outer, outer_n, NULL, 0, &q->non_zero, fill);
			return;
		}
		if (hollow) {
			hole_n = phppq_outline(q, x + bw, y + bw, width - 2 * bw, height - 2 * bw, inner, hole);
			if (hole_n == 0) {
				RETURN_THROWS();
			}
			phppq_record_path(q, hole, hole_n, NULL, 0, &q->non_zero, fill);
		}
		phppq_record_path(q, outer, outer_n, hollow ? hole : NULL, hole_n, &q->even_odd, border);
		return;
	}

	scale = q->m[0];
	if (!phppq_point(q, x, y, "x", "y", a) || !phppq_point(q, x + width, y + height, "x", "y", b)) {
		RETURN_THROWS();
	}
	if (phppq_clip_empty(q)) {
		return;
	}
	if (bw <= 0) {
		phppq_record_round(q, phppq_kind_roundrect, a[0], a[1], b[0], b[1], radii, scale, NULL, fill);
		return;
	}
	inset = bw * scale;
	if (hollow) {
		phppq_record_round(q, phppq_kind_roundrect, a[0] + inset, a[1] + inset, b[0] - inset, b[1] - inset, inner, scale, NULL, fill);
	}
	phppq_record_round(q, phppq_kind_roundring, a[0], a[1], b[0], b[1], radii, scale, &inset, border);
}

PHP_METHOD(PixelQueue, shadow)
{
	double x, y, width, height, radius, blur, dx, dy, scale, a[2], b[2], half;
	zend_long rgba;
	phppq_queue *q;
	zval command;
	zend_array *list;

	ZEND_PARSE_PARAMETERS_START(9, 9)
		Z_PARAM_DOUBLE(x)
		Z_PARAM_DOUBLE(y)
		Z_PARAM_DOUBLE(width)
		Z_PARAM_DOUBLE(height)
		Z_PARAM_DOUBLE(radius)
		Z_PARAM_DOUBLE(blur)
		Z_PARAM_LONG(rgba)
		Z_PARAM_DOUBLE(dx)
		Z_PARAM_DOUBLE(dy)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	if (!phppq_open(q, "shadow") || !phppq_finite(x, "x") || !phppq_finite(y, "y") || !phppq_finite(width, "width") || !phppq_finite(height, "height")
		|| !phppq_finite(radius, "radius") || !phppq_finite(blur, "blur") || !phppq_finite(dx, "dx") || !phppq_finite(dy, "dy")) {
		RETURN_THROWS();
	}
	if (width <= 0 || height <= 0) {
		return;
	}
	if (!phppq_even(q)) {
		phppq_refuse(PQ_UNEVEN, "shadow", 0.0);
		RETURN_THROWS();
	}
	scale = q->m[0];
	blur = blur > 0.0 ? blur : 0.0;
	if (!phppq_point(q, x + dx, y + dy, "x", "y", a) || !phppq_point(q, x + width + dx, y + height + dy, "x", "y", b)) {
		RETURN_THROWS();
	}
	half = (height < width ? height : width) / 2;
	if (half < radius) {
		radius = half;
	}
	radius = radius > 0.0 ? radius : 0.0;
	if (phppq_clip_empty(q)) {
		return;
	}
	blur *= scale;
	list = zend_new_array(9);
	phppq_add_kind(list, phppq_kind_shadow);
	phppq_add_double(list, a[0]);
	phppq_add_double(list, a[1]);
	phppq_add_double(list, b[0]);
	phppq_add_double(list, b[1]);
	phppq_add_double(list, radius * scale);
	phppq_add_double(list, blur);
	phppq_add_long(list, rgba);
	phppq_add_copy(list, &q->clip_value);
	ZVAL_ARR(&command, list);
	phppq_meta_reach(q, phppq_frame_add(&q->open, &command), a[0] - blur - 1.0, a[1] - blur - 1.0, b[0] + blur + 1.0, b[1] + blur + 1.0);
}

PHP_METHOD(PixelQueue, glyphs)
{
	HashTable *ink_list = NULL;
	double x, y, ink[4], origin[2], corners[8], reach[4];
	zend_long rgba;
	phppq_queue *q;
	zval *runs, command, entry;
	zend_array *list, *matrix, *box;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_ARRAY(runs)
		Z_PARAM_DOUBLE(x)
		Z_PARAM_DOUBLE(y)
		Z_PARAM_ARRAY_HT_OR_NULL(ink_list)
		Z_PARAM_LONG(rgba)
	ZEND_PARSE_PARAMETERS_END();

	if (ink_list != NULL && !phppq_numbers(ink_list, 4, ink, 4, "null or [x0, y0, x1, y1], four finite numbers")) {
		RETURN_THROWS();
	}
	q = PHPPQ_THIS();
	if (!phppq_open(q, "text") || !phppq_finite(x, "x") || !phppq_finite(y, "y") || !phppq_point(q, x, y, "x", "y", origin)) {
		RETURN_THROWS();
	}
	if (ink_list == NULL || phppq_clip_empty(q)) {
		return;
	}
	/* the ink's box through the transform, grown by a pixel: anti-aliased edges, and each glyph moved to a whole pixel */
	if (!phppq_point(q, x + ink[0], y + ink[1], "x", "y", corners) || !phppq_point(q, x + ink[2], y + ink[1], "x", "y", corners + 2)
		|| !phppq_point(q, x + ink[2], y + ink[3], "x", "y", corners + 4) || !phppq_point(q, x + ink[0], y + ink[3], "x", "y", corners + 6)) {
		RETURN_THROWS();
	}
	phppq_reach(corners, 4, reach, false);

	list = zend_new_array(6);
	phppq_add_kind(list, phppq_kind_glyphs);
	phppq_add_copy(list, runs);
	matrix = zend_new_array(6);
	phppq_add_double(matrix, q->m[0]);
	phppq_add_double(matrix, q->m[1]);
	phppq_add_double(matrix, q->m[2]);
	phppq_add_double(matrix, q->m[3]);
	phppq_add_double(matrix, origin[0]);
	phppq_add_double(matrix, origin[1]);
	ZVAL_ARR(&entry, matrix);
	zend_hash_next_index_insert_new(list, &entry);
	phppq_add_long(list, rgba);
	box = zend_new_array(4);
	phppq_add_double(box, reach[0] - 1.0);
	phppq_add_double(box, reach[1] - 1.0);
	phppq_add_double(box, reach[2] + 1.0);
	phppq_add_double(box, reach[3] + 1.0);
	ZVAL_ARR(&entry, box);
	zend_hash_next_index_insert_new(list, &entry);
	phppq_add_copy(list, &q->clip_value);
	ZVAL_ARR(&command, list);
	phppq_meta_reach(q, phppq_frame_add(&q->open, &command), reach[0] - 1.0, reach[1] - 1.0, reach[2] + 1.0, reach[3] + 1.0);
}

/* ------------------------------------------------------------------- boxes */

/* A number out of a list entry; false when it is not one. */
static zend_always_inline bool phppq_number(zval *value, double *out)
{
	ZVAL_DEREF(value);
	if (Z_TYPE_P(value) == IS_DOUBLE) {
		*out = Z_DVAL_P(value);
		return true;
	}
	if (Z_TYPE_P(value) == IS_LONG) {
		*out = (double) Z_LVAL_P(value);
		return true;
	}
	return false;
}

/* Join a list of [x, y] into reach; false when an entry is not a pair of numbers. */
static bool phppq_reach_of_points(zval *points, double reach[4], bool *set)
{
	zval *point;

	ZVAL_DEREF(points);
	if (Z_TYPE_P(points) != IS_ARRAY) {
		return false;
	}
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(points), point) {
		zval *zx, *zy;
		double xy[2];

		ZVAL_DEREF(point);
		if (Z_TYPE_P(point) != IS_ARRAY
			|| (zx = zend_hash_index_find(Z_ARRVAL_P(point), 0)) == NULL || (zy = zend_hash_index_find(Z_ARRVAL_P(point), 1)) == NULL
			|| !phppq_number(zx, &xy[0]) || !phppq_number(zy, &xy[1])) {
			return false;
		}
		phppq_reach(xy, 1, reach, *set);
		*set = true;
	} ZEND_HASH_FOREACH_END();
	return true;
}

/* The reach of a command that was appended with its points or its spans left in it; false when it has none. */
static bool phppq_reach_in_command(zval *command, uint8_t how, double grow, double reach[4])
{
	zval *second;
	bool set = false;

	if (Z_TYPE_P(command) != IS_ARRAY || (second = zend_hash_index_find(Z_ARRVAL_P(command), 1)) == NULL) {
		return false;
	}
	ZVAL_DEREF(second);
	if (how == PQ_HOW_SPANS) {
		const unsigned char *bytes;
		size_t length;
		zend_long x0 = ZEND_LONG_MAX, y0 = ZEND_LONG_MAX, x1 = 0, y1 = 0;

		if (Z_TYPE_P(second) != IS_STRING) {
			return false;
		}
		bytes = (const unsigned char *) Z_STRVAL_P(second);
		length = Z_STRLEN_P(second);
		if (length < 7) {
			return false;
		}
		for (size_t at = 0; at + 7 <= length; at += 7) {
			zend_long y = bytes[at] | (bytes[at + 1] << 8);
			zend_long x = bytes[at + 2] | (bytes[at + 3] << 8);
			zend_long right = x + (bytes[at + 4] | (bytes[at + 5] << 8));

			if (x < x0) {
				x0 = x;
			}
			if (right > x1) {
				x1 = right;
			}
			if (y < y0) {
				y0 = y;
			}
			if (y >= y1) {
				y1 = y + 1;
			}
		}
		reach[0] = (double) x0;
		reach[1] = (double) y0;
		reach[2] = (double) x1;
		reach[3] = (double) y1;
		return true;
	}

	if (how == PQ_HOW_POINTS) {
		if (!phppq_reach_of_points(second, reach, &set)) {
			return false;
		}
	} else {
		zval *contour;

		if (Z_TYPE_P(second) != IS_ARRAY) {
			return false;
		}
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(second), contour) {
			if (!phppq_reach_of_points(contour, reach, &set)) {
				return false;
			}
		} ZEND_HASH_FOREACH_END();
	}
	if (!set) {
		return false;
	}
	reach[0] -= grow;
	reach[1] -= grow;
	reach[2] += grow;
	reach[3] += grow;
	return true;
}

/* The whole pixels command i of a frame writes; false when it writes none. */
static bool phppq_box(const pq_frame *f, uint32_t i, int64_t sw, int64_t sh, pq_rect *out)
{
	const pq_meta *m = &f->meta[i];
	pq_piece piece = m->piece;
	bool have;

	switch (m->how) {
		case PQ_HOW_WHOLE:
			out->x = 0;
			out->y = 0;
			out->w = sw;
			out->h = sh;
			return true;
		case PQ_HOW_ONE:
		case PQ_HOW_PIECES:
			break;
		default: {
			double reach[4];
			zval *command = zend_hash_index_find(f->list, i);

			if (command == NULL || !phppq_reach_in_command(command, m->how, m->piece.x0, reach)) {
				return false;
			}
			piece.x0 = reach[0];
			piece.y0 = reach[1];
			piece.x1 = reach[2];
			piece.y1 = reach[3];
			break;
		}
	}
	have = pq_piece_box(&piece, sw, sh, out);
	if (m->extra != PQ_NO_EXTRA && f->extra[m->extra].has_second) {
		pq_rect second;

		if (pq_piece_box(&f->extra[m->extra].second, sw, sh, &second)) {
			*out = have ? pq_union(*out, second) : second;
			have = true;
		}
	}
	return have;
}

/* The surface, the grain and the extra rects of drawn() and changed(); false with a ValueError thrown. */
static bool phppq_measure(zend_long sw, zend_long sh, zend_long uw, zend_long uh, zend_long lw, zend_long lh, HashTable *also, pq_grain *grain, pq_rects *extra)
{
	const zend_long limit = (zend_long) PQ_LIMIT;
	const zend_long sizes[6] = {sw, sh, uw, uh, lw, lh};
	zval *value;
	uint32_t count = also == NULL ? 0 : zend_hash_num_elements(also), i = 0;
	pq_rect rect = {0, 0, 0, 0};

	for (uint32_t k = 0; k < 6; k++) {
		if (sizes[k] < 1 || sizes[k] > limit) {
			zend_argument_value_error(k + 1, "must be between 1 and %d", (int) limit);
			return false;
		}
	}
	grain->unit_w = uw;
	grain->unit_h = uh;
	grain->limit_w = lw;
	grain->limit_h = lh;

	if (count == 0) {
		return true;
	}
	if (count % 4 != 0 || !zend_array_is_list(also)) {
		zend_argument_value_error(7, "must be a list of rects, four integers each");
		return false;
	}
	ZEND_HASH_FOREACH_VAL(also, value) {
		zend_long n;

		ZVAL_DEREF(value);
		if (Z_TYPE_P(value) != IS_LONG || (n = Z_LVAL_P(value)) < -limit || n > limit) {
			zend_argument_value_error(7, "must be a list of rects, four integers each, within the limit");
			return false;
		}
		switch (i++ % 4) {
			case 0: rect.x = n; break;
			case 1: rect.y = n; break;
			case 2: rect.w = n; break;
			default:
				rect.h = n;
				pq_rects_push(extra, rect);
				break;
		}
	} ZEND_HASH_FOREACH_END();
	return true;
}

static void phppq_return_rects(zval *return_value, const pq_rects *rects)
{
	zend_array *list = zend_new_array(rects->n * 4);

	for (uint32_t i = 0; i < rects->n; i++) {
		phppq_add_long(list, (zend_long) rects->v[i].x);
		phppq_add_long(list, (zend_long) rects->v[i].y);
		phppq_add_long(list, (zend_long) rects->v[i].w);
		phppq_add_long(list, (zend_long) rects->v[i].h);
	}
	RETVAL_ARR(list);
}

PHP_METHOD(PixelQueue, drawn)
{
	zend_long sw, sh, uw, uh, lw, lh;
	HashTable *also = NULL;
	phppq_queue *q;
	pq_grain grain;
	pq_rects extra, merged;

	ZEND_PARSE_PARAMETERS_START(6, 7)
		Z_PARAM_LONG(sw)
		Z_PARAM_LONG(sh)
		Z_PARAM_LONG(uw)
		Z_PARAM_LONG(uh)
		Z_PARAM_LONG(lw)
		Z_PARAM_LONG(lh)
		Z_PARAM_OPTIONAL
		Z_PARAM_ARRAY_HT(also)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	pq_rects_init(&extra);
	pq_rects_init(&merged);
	if (!phppq_measure(sw, sh, uw, uh, lw, lh, also, &grain, &extra)) {
		pq_rects_free(&extra);
		RETURN_THROWS();
	}
	for (uint32_t i = 0; i < q->last.n; i++) {
		pq_rect box;

		if (phppq_box(&q->last, i, sw, sh, &box)) {
			pq_merge_one(&merged, box, grain);
		}
	}
	pq_merge(&extra, &merged, grain);
	phppq_return_rects(return_value, &merged);
	pq_rects_free(&extra);
	pq_rects_free(&merged);
}

PHP_METHOD(PixelQueue, changed)
{
	zend_long sw, sh, uw, uh, lw, lh;
	HashTable *also = NULL;
	phppq_queue *q;
	pq_grain grain;
	pq_rects extra, differing, regions, outers;
	pq_rect surface, box;
	uint32_t count;
	bool grew;

	ZEND_PARSE_PARAMETERS_START(6, 7)
		Z_PARAM_LONG(sw)
		Z_PARAM_LONG(sh)
		Z_PARAM_LONG(uw)
		Z_PARAM_LONG(uh)
		Z_PARAM_LONG(lw)
		Z_PARAM_LONG(lh)
		Z_PARAM_OPTIONAL
		Z_PARAM_ARRAY_HT(also)
	ZEND_PARSE_PARAMETERS_END();

	q = PHPPQ_THIS();
	pq_rects_init(&extra);
	pq_rects_init(&differing);
	pq_rects_init(&regions);
	pq_rects_init(&outers);
	if (!phppq_measure(sw, sh, uw, uh, lw, lh, also, &grain, &extra)) {
		pq_rects_free(&extra);
		RETURN_THROWS();
	}

	/* By position: an equal command changed nothing; any other marks its box and the box of the one it replaced. */
	count = q->before.n > q->last.n ? q->before.n : q->last.n;
	for (uint32_t i = 0; i < count; i++) {
		bool was = i < q->before.n, is = i < q->last.n;

		if (was && is && !q->last.meta[i].restless) {
			zval *old = zend_hash_index_find(q->before.list, i), *new = zend_hash_index_find(q->last.list, i);
			bool equal = old != NULL && new != NULL && zend_compare(old, new) == 0;

			if (EG(exception)) {
				pq_rects_free(&extra);
				pq_rects_free(&differing);
				RETURN_THROWS();
			}
			if (equal) {
				continue;
			}
		}
		if (was && phppq_box(&q->before, i, sw, sh, &box)) {
			pq_merge_one(&differing, box, grain);
		}
		if (is && phppq_box(&q->last, i, sw, sh, &box)) {
			pq_merge_one(&differing, box, grain);
		}
	}
	pq_merge(&differing, &regions, grain);
	pq_merge(&extra, &regions, grain);

	/* What reads under itself, grown by its reach and kept on the surface. */
	surface.x = 0;
	surface.y = 0;
	surface.w = sw;
	surface.h = sh;
	for (uint32_t i = 0; i < q->last.n; i++) {
		const pq_meta *m = &q->last.meta[i];

		if (m->extra != PQ_NO_EXTRA && q->last.extra[m->extra].reads) {
			const pq_extra *e = &q->last.extra[m->extra];
			int64_t by = (int64_t) ceil(e->reach);
			pq_rect grown;

			grown.x = e->area.x - by;
			grown.y = e->area.y - by;
			grown.w = e->area.w + 2 * by;
			grown.h = e->area.h + 2 * by;
			if (pq_intersect(grown, surface, &grown)) {
				pq_rects_push(&outers, grown);
			}
		}
	}
	/* One that overlaps a changed rect is redrawn whole, and may then reach the next. */
	do {
		uint32_t kept = 0;

		grew = false;
		for (uint32_t i = 0; i < outers.n; i++) {
			bool touched = false;

			for (uint32_t r = 0; r < regions.n; r++) {
				if (pq_intersect(regions.v[r], outers.v[i], NULL)) {
					touched = true;
					break;
				}
			}
			if (touched) {
				pq_rects again;

				pq_rects_init(&again);
				pq_merge(&regions, &again, grain);
				pq_merge_one(&again, outers.v[i], grain);
				pq_rects_free(&regions);
				regions = again;
				grew = true;
			} else {
				outers.v[kept++] = outers.v[i];
			}
		}
		outers.n = kept;
	} while (grew);

	phppq_return_rects(return_value, &regions);
	pq_rects_free(&extra);
	pq_rects_free(&differing);
	pq_rects_free(&regions);
	pq_rects_free(&outers);
}
