#include "ruby.h"

//
// interface
//

// the tree itself
typedef struct kdtree_data
{
    int root;
    int len;
    struct kdtree_node *nodes;
} kdtree_data;

// a node in the tree
typedef struct kdtree_node
{
    float x, y;
    int id;
    int left;
    int right;
} kdtree_node;

// a result node from kdtree_nearestk0
typedef struct kresult {
    int index;
    float distance;
} kresult;

// helper macro for digging out our struct
#define KDTREEP \
    struct kdtree_data *kdtreep; \
    Data_Get_Struct(kdtree, struct kdtree_data, kdtreep);

// kdtree public methods
static VALUE kdtree_alloc(VALUE klass);
static void kdtree_free(struct kdtree_data *kdtreep);
static VALUE kdtree_initialize(VALUE kdtree, VALUE points);
static VALUE kdtree_nearest(VALUE kdtree, VALUE x, VALUE y);
static VALUE kdtree_nearestk(VALUE kdtree, VALUE x, VALUE y, VALUE k);
static VALUE kdtree_persist(VALUE kdtree, VALUE io);
static VALUE kdtree_to_s(VALUE kdtree);

// kdtree helpers
static int kdtree_build(struct kdtree_data *kdtreep, int min, int max, int depth);
static void kdtree_nearest0(struct kdtree_data *kdtreep, int i, float x, float y, int depth, int *n_index, float *n_dist);
static void kdtree_nearestk0(struct kdtree_data *kdtreep, int i, float x, float y, int k, int depth, kresult *k_list, int *k_len, float *k_dist);

// io helpers
static void read_all(VALUE io, void *buf, int len);
static void write_all(VALUE io, const void *buf, int len);

#define KDTREE_MAGIC "KdTr"

// ids
static ID id_read, id_write, id_binmode;

//
// implementation
//

static VALUE kdtree_alloc(VALUE klass)
{
    struct kdtree_data *kdtreep;
    VALUE obj = Data_Make_Struct(klass, struct kdtree_data, 0, kdtree_free, kdtreep);
    kdtreep->root = -1;
    return obj;
}

static void kdtree_free(struct kdtree_data *kdtreep)
{
    if (kdtreep) {
        free(kdtreep->nodes);
    }
}

/*
 * call-seq:
 *   Kdtree.new(points)    => kdtree
 *   Kdtree.new(io)        => kdtree
 *
 * Returns a new <code>Kdtree</code>. To construct a tree, pass an array of
 * <i>points</i>. Each point should be an array of the form <code>[x, y,
 * id]</code>, where <i>x</i> and <i>y</i> are floats and <i>id</i> is an
 * integer. The <i>id</i> is arbitrary and will be returned to you whenever you
 * search with nearest or nearestk.
 *
 *   # create a new tree
 *   points = []
 *   points << [47.6, -122.3, 1] # Seattle
 *   points << [40.7, -74.0, 2]  # New York
 *   kd = Kdtree.new(points)
 *
 * Alternately, you can pass in an <i>IO</i> object containing a persisted
 * kdtree. This makes it possible to build the tree in advance, persist it, and
 * start it up quickly later. See persist for more information.
 */
static VALUE kdtree_initialize(VALUE kdtree, VALUE arg)
{
    KDTREEP;

    if (TYPE(arg) == T_ARRAY) {
        // init from array of pints
        VALUE points = arg;
        int i;
        kdtreep->len = RARRAY_LEN(points);
        kdtreep->nodes = ALLOC_N(struct kdtree_node, kdtreep->len);

        for (i = 0; i < RARRAY_LEN(points); ++i) {
            struct kdtree_node *n = kdtreep->nodes + i;

            VALUE ptr = rb_ary_entry(points, i);
            VALUE v = rb_check_array_type(ptr);
            if (NIL_P(v) || RARRAY_LEN(v) != 3) {
                continue;
            }
            n->x = NUM2DBL(rb_ary_entry(v, 0));
            n->y = NUM2DBL(rb_ary_entry(v, 1));
            n->id = NUM2INT(rb_ary_entry(v, 2));
        }

        // now build the tree
        kdtreep->root = kdtree_build(kdtreep, 0, kdtreep->len, 0);
    } else if (rb_respond_to(arg, rb_intern("read"))) {
        VALUE io = arg;
        char buf[4];
        if (rb_respond_to(io, id_binmode)) {
            rb_funcall(io, id_binmode, 0);
        }

        // check magic
        read_all(io, buf, 4);
        if (memcmp(KDTREE_MAGIC, buf, 4) != 0) {
            rb_raise(rb_eRuntimeError, "wrong magic number in kdtree file");
        }

        // read start of the struct
        read_all(io, kdtreep, sizeof(struct kdtree_data) - sizeof(struct kdtree_node *));

        // read the nodes
        kdtreep->nodes = ALLOC_N(struct kdtree_node, kdtreep->len);
        read_all(io, kdtreep->nodes, sizeof(struct kdtree_node) * kdtreep->len);
    } else {
        rb_raise(rb_eTypeError, "array or IO required to init Kdtree");
    }

    return kdtree;
}

static int comparex(const void *pa, const void *pb)
{
    float a = ((const struct kdtree_node*)pa)->x;
    float b = ((const struct kdtree_node*)pb)->x;
    return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

static int comparey(const void *pa, const void *pb)
{
    float a = ((const struct kdtree_node*)pa)->y;
    float b = ((const struct kdtree_node*)pb)->y;
    return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

static int kdtree_build(struct kdtree_data *kdtreep, int min, int max, int depth)
{
    int(*compar)(const void *, const void *);
    struct kdtree_node *m;
    int median;
    if (max <= min) {
        return -1;
    }

    // sort nodes from min to max
    compar = (depth % 2) ? comparex : comparey;
    qsort(kdtreep->nodes + min, max - min, sizeof(struct kdtree_node), compar);

    median = (min + max) / 2;
    m = kdtreep->nodes + median;
    m->left = kdtree_build(kdtreep, min, median, depth + 1);
    m->right = kdtree_build(kdtreep, median + 1, max, depth + 1);
    return median;
}

/*
 * call-seq:
 *   kd.nearest(x, y)    => id
 *
 * Finds the point closest to <i>x</i>, <i>y</i> and returns the id for that
 * point. Returns -1 if the tree is empty.
 *
 *   points = []
 *   points << [47.6, -122.3, 1] # Seattle
 *   points << [40.7, -74.0, 2]  # New York
 *   kd = Kdtree.new(points)
 *
 *   # which city is closest to Portland?
 *   kd.nearest(45.5, -122.8) #=> 1
 *   # which city is closest to Boston?
 *   kd.nearest(42.4, -71.1)  #=> 2
 */
static VALUE kdtree_nearest(VALUE kdtree, VALUE x, VALUE y)
{
    int n_index;
    float n_dist;
    KDTREEP;

    n_index = -1;
    n_dist = INT_MAX;

    kdtree_nearest0(kdtreep, kdtreep->root, NUM2DBL(x), NUM2DBL(y), 0, &n_index, &n_dist);
    if (n_index == -1) {
        return -1;
    }
    return INT2NUM((kdtreep->nodes + n_index)->id);
}

static void kdtree_nearest0(struct kdtree_data *kdtreep, int i, float x, float y, int depth, int *n_index, float *n_dist)
{
    struct kdtree_node *n;
    float ad;
    int near, far;
    float dx;

    if (i == -1) {
        return;
    }

    n = kdtreep->nodes + i;

    ad = (depth % 2) ? (x - n->x) : (y - n->y);

    //
    // recurse near, and perhaps far as well
    //

    if (ad <= 0) {
        near = n->left; far = n->right;
    } else {
        near = n->right; far = n->left;
    }
    kdtree_nearest0(kdtreep, near,  x, y, depth + 1, n_index, n_dist);
    if (ad * ad < *n_dist) {
        kdtree_nearest0(kdtreep, far, x, y, depth + 1, n_index, n_dist);
    }

    //
    // do we beat the old distance?
    //

    dx = (x - n->x) * (x - n->x);
    if (dx < *n_dist) {
        float d = dx + ((y - n->y) * (y - n->y));
        if (d < *n_dist) {
            *n_index = i;
            *n_dist = d;
        }
    }
}

//
// nearestK
//

#define MAX_K 255

/*
 * call-seq:
 *   kd.nearestk(x, y, k)    => array
 *
 * Finds the <i>k</i> points closest to <i>x</i>, <i>y</i>. Returns an array of
 * ids, sorted by distance. Returns an empty array if the tree is empty. Note
 * that <i>k</i> is capped at 255.
 *
 *   points = []
 *   points << [47.6, -122.3, 1] # Seattle
 *   points << [45.5, -122.8, 2] # Portland
 *   points << [40.7, -74.0,  3] # New York
 *   kd = Kdtree.new(points)
 *
 *   # which two cities are closest to San Francisco?
 *   kd.nearest(34.1, -118.2) #=> [2, 1]
 */
static VALUE kdtree_nearestk(VALUE kdtree, VALUE x, VALUE y, VALUE k)
{
    // note I leave an extra slot here at the end because of the way our binary insert works
    kresult k_list[MAX_K + 1];
    int k_len = 0;
    float k_dist = INT_MAX;
    int ki = NUM2INT(k);
    VALUE ary;
    int i;
    KDTREEP;

    if (ki < 1) {
        ki = 1;
    } else if (ki > MAX_K) {
        ki = MAX_K;
    }
    kdtree_nearestk0(kdtreep, kdtreep->root, NUM2DBL(x), NUM2DBL(y), ki, 0, k_list, &k_len, &k_dist);

    // convert result to ruby array
    ary = rb_ary_new();
    for (i = 0; i < k_len; ++i) {
        rb_ary_push(ary, INT2NUM(kdtreep->nodes[k_list[i].index].id));
    }
    return ary;
}

static void kdtree_nearestk0(struct kdtree_data *kdtreep, int i, float x, float y, int k, int depth, kresult *k_list, int *k_len, float *k_dist)
{
    struct kdtree_node *n;
    float ad;
    int near, far;
    float dx;
    int lo, hi;

    if (i == -1) {
        return;
    }

    n = kdtreep->nodes + i;

    ad = (depth % 2) ? (x - n->x) : (y - n->y);

    //
    // recurse near, and then perhaps far as well
    //

    if (ad <= 0) {
        near = n->left; far = n->right;
    } else {
        near = n->right; far = n->left;
    }
    kdtree_nearestk0(kdtreep, near,  x, y, k, depth + 1, k_list, k_len, k_dist);
    if (ad * ad < *k_dist) {
        kdtree_nearestk0(kdtreep, far, x, y, k, depth + 1, k_list, k_len, k_dist);
    }

    //
    // do we beat the old distance?
    //

    dx = (x - n->x) * (x - n->x);
    if (dx < *k_dist) {
        float d = dx + ((y - n->y) * (y - n->y));
        if (d < *k_dist) {
            //
            // find spot to insert
            //
            lo = 0, hi = *k_len;
            while (lo < hi) {
                int mid = (lo + hi) / 2;
                if (k_list[mid].distance < d) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }

            //
            // insert
            //

            memmove(k_list + lo + 1, k_list + lo, (*k_len - lo) * sizeof(struct kresult));
            k_list[lo].index = i;
            k_list[lo].distance = d;

            //
            // adjust len/dist if necessary
            //

            if (*k_len < k) {
                ++(*k_len);
            } else {
                *k_dist = k_list[k - 1].distance;
            }
        }
    }
}

/*
 * call-seq:
 *   kd.persist(io)
 *
 * Writes the tree out to <i>io</i> so you can quickly load it later with
 * Kdtree.new. This avoids the startup cost of initializing a tree. Apart from a
 * small header, the size of the file is proportional to the number of points,
 * requiring 20 bytes per point.
 *
 * This file is <b>NOT PORTABLE</b> across different architectures due to endian
 * issues.
 *
 *   points = []
 *   points << [47.6, -122.3, 1] # Seattle
 *   points << [45.5, -122.8, 2] # Portland
 *   points << [40.7, -74.0,  3] # New York
 *   kd = Kdtree.new(points)
 *
 *   # persist the tree to disk
 *   File.open("treefile", "w") { |f| kd.persist(f) }
 *
 *   ...
 *
 *   # later, read the tree from disk
 *   kd2 = File.open("treefile") { |f| Kdtree.new(f) }
 */
static VALUE kdtree_persist(VALUE kdtree, VALUE io)
{
    VALUE str;
    KDTREEP;

    if (!rb_respond_to(io, rb_intern("write"))) {
        rb_raise(rb_eTypeError, "instance of IO needed");
    }
    if (rb_respond_to(io, id_binmode)) {
        rb_funcall(io, id_binmode, 0);
    }

    write_all(io, KDTREE_MAGIC, 4);
    write_all(io, kdtreep, sizeof(struct kdtree_data) - sizeof(struct kdtree_node *));
    write_all(io, kdtreep->nodes, sizeof(struct kdtree_node) * kdtreep->len);
    return io;
}

/*
 * call-seq:
 *   kd.to_s     => string
 *
 * A string that tells you a bit about the tree.
 */
static VALUE kdtree_to_s(VALUE kdtree)
{
    char buf[256];
    KDTREEP;

    sprintf(buf, "#<%s:%p nodes=%d>", rb_obj_classname(kdtree), (void*)kdtree, kdtreep->len);
    return rb_str_new(buf, strlen(buf));
}

//
// io helpers
//

static void read_all(VALUE io, void *buf, int len)
{
    VALUE string = rb_funcall(io, id_read, 1, INT2NUM(len));
    if (NIL_P(string) || RSTRING_LEN(string) != len) {
        rb_raise(rb_eEOFError, "end of file reached");
    }
    memcpy(buf, RSTRING_PTR(string), len);
}

static void write_all(VALUE io, const void *buf, int len)
{
    rb_funcall(io, id_write, 1, rb_str_new(buf, len));
}

//
// entry point
//

/*
 *  Kdtree is an insanely fast data structure for finding the nearest
 *  neighbor(s) to a given point. This implementation only supports 2d
 *  points. Also, it only supports static points - there is no way to edit the
 *  tree after it has been initialized. Kdtree should scale to millions of
 *  points, though it's only been tested with around 1 million.
 *
 *  Once the tree is constructed, it can be searched with nearest and nearestk.
 *
 *  To avoid the startup costs associated with creating a new tree, use persist
 *  to write the tree to disk. You can then construct the tree later from that
 *  file.
 *
 *   points = []
 *   points << [47.6, -122.3, 1] # Seattle
 *   points << [45.5, -122.8, 2] # Portland
 *   points << [40.7, -74.0,  3] # New York
 *   kd = Kdtree.new(points)
 *
 *   # which city is closest to San Francisco?
 *   kd.nearest(34.1, -118.2) #=> 2
 *   # which two cities are closest to San Francisco?
 *   kd.nearest(34.1, -118.2) #=> [2, 1]
 *
 * For more information on kd trees, see:
 *
 * http://en.wikipedia.org/wiki/Kd-tree
 */
void Init_kdtree()
{
    static VALUE clazz;

    clazz = rb_define_class("Kdtree", rb_cObject);

    rb_define_alloc_func(clazz, kdtree_alloc);
    rb_define_method(clazz, "initialize", kdtree_initialize, 1);
    rb_define_method(clazz, "nearest", kdtree_nearest, 2);
    rb_define_method(clazz, "nearestk", kdtree_nearestk, 3);
    rb_define_method(clazz, "persist", kdtree_persist, 1);
    rb_define_method(clazz, "to_s", kdtree_to_s, 0);

    // function ids
    id_binmode = rb_intern("binmode");
    id_read = rb_intern("read");
    id_write = rb_intern("write");
}
