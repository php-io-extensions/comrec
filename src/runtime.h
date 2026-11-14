/*
 * What the class source shares with the module: the Zend object that holds a
 * queue, and the class entries.
 */
#ifndef PHPPQ_RUNTIME_H
#define PHPPQ_RUNTIME_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_pixel_queue.h"
#include "boxes.h"

/* A turned ellipse or rounded corner becomes a polygon that strays from it by less than this. */
#define PQ_FLATNESS 0.1

/* Where a command writes: how its box is found. PQ_HOW_ONE is the queue's own shapes. */
enum {
	PQ_HOW_WHOLE = 1,
	PQ_HOW_CONTOURS = 2,
	PQ_HOW_POINTS = 3,
	PQ_HOW_SPANS = 4,
	PQ_HOW_PIECES = 5,
	PQ_HOW_ONE = 6
};

/* Why a shape was refused: PixelQueueException's code. */
enum {
	PQ_NOT_RECORDING = 1,
	PQ_NOT_FINITE = 2,
	PQ_PAST_LIMIT = 3,
	PQ_BAD_RADII = 4,
	PQ_UNEVEN = 5,
	PQ_BAD_MATRIX = 6
};

#define PQ_NO_EXTRA UINT32_MAX

/* What the queue knows of one command, beside the command itself. */
typedef struct {
	uint8_t how;       /* PQ_HOW_* */
	uint8_t restless;  /* never equal to the command before it */
	uint32_t extra;    /* its pq_extra in the frame, or PQ_NO_EXTRA */
	pq_piece piece;    /* ONE and PIECES: its reach. CONTOURS and POINTS: x0 is the grow. All: its clip. */
} pq_meta;

/* What only a few commands have. */
typedef struct {
	bool has_second;
	pq_piece second;   /* a second reach, with a clip of its own */
	bool reads;
	pq_rect area;      /* what it reads under itself */
	double reach;      /* and how far around that */
} pq_extra;

/* A list of commands: a frame, or a layer's children. */
typedef struct {
	zend_array *list;  /* the commands, a packed list; NULL when there is none */
	pq_meta *meta;
	uint32_t n, cap;
	pq_extra *extra;
	uint32_t nextra, capextra;
} pq_frame;

/* What push() keeps. */
typedef struct {
	double m[6];
	zval clip_value;
	pq_rect clip;
} pq_saved;

/* What enter() sets aside. */
typedef struct {
	pq_frame frame;
	zval clip_value;
	pq_rect clip;
	uint32_t base;
} pq_layer;

typedef struct {
	bool recording;
	pq_frame open;        /* the frame being recorded, or the innermost layer */
	pq_frame last;        /* the last finished frame */
	pq_frame before;      /* the frame finished before it */

	double m[6];          /* a, b, c, d, e, f */
	zval clip_value;      /* what commands carry as their clip */
	pq_rect clip;

	pq_saved *stack;
	uint32_t top, base, stack_cap;   /* pushes below base belong to what is around the open layer */

	pq_layer *layers;
	uint32_t nlayers, layers_cap;

	zval non_zero, even_odd;
	zend_object std;
} phppq_queue;

static zend_always_inline phppq_queue *phppq_queue_from(zend_object *obj)
{
	return (phppq_queue *) ((char *) obj - XtOffsetOf(phppq_queue, std));
}

extern zend_class_entry *phppq_ce_PixelQueue;
extern zend_class_entry *phppq_ce_PixelQueueException;

void phppq_register_PixelQueue(void);
void phppq_register_PixelQueueException(void);

#endif
