## Kdtree

A kd tree is a data structure that recursively partitions the world in order to rapidly answer nearest neighbor queries. A generic kd tree can support any number of dimensions, and can return either the nearest neighbor or a set of N nearest neighbors.

This gem is a blazingly fast, native, 2d kdtree. It's specifically built to find the nearest neighbor when searching millions of points. It's used in production at Urbanspoon and several other companies.

The first version of this gem was released back in 2009. See the original [blog post](http://gurge.com/2009/10/22/ruby-nearest-neighbor-fast-kdtree-gem/) for the full story. Wikipedia has a great [article on kdtrees](http://en.wikipedia.org/wiki/K-d_tree).

Note: kdtree 0.3 obsoletes these forks: ghazel-kdtree, groupon-kdtree, tupalo-kdree. Thanks guys!

### Usage

First, install kdtree:

```sh
$ sudo gem install kdtree
```

It's easy to use:

* **Kdtree.new(points)** - construct a new tree. Each point should be of the form `[x, y, id]`, where `x/y` are floats and `id` is an int. Not a string, not an object, **just an int**.
* **kd.nearest(x, y)** - find the nearest point. Returns an id.
* **kd.nearestk(x, y, k)** - find the nearest `k` points. Returns an array of ids.

For example:

```ruby
# construct the tree
points = []
points << [47.6, -122.3, 1] # Seattle id=1
points << [45.5, -122.8, 2] # Portland id=2
points << [40.7, -74.0,  3] # New York id=3
kd = Kdtree.new(points)

# which city is closest to San Francisco?
p kd.nearest(34.1, -118.2) # => 2
# which two cities are closest to San Francisco?
p kd.nearestk(34.1, -118.2, 2) # => [2, 1]
```

Also, I made it possible to **persist** the tree to disk and load it later. That way you can calculate the tree offline and load it quickly at some future point. Loading a persisted tree w/ 1 millions points takes half a second, as opposed to the 3.5 second build time shown below. At Urbanspoon we persist the tree and rsync it out to other machines. For example:

```ruby
File.open("treefile", "w") { |f| kd.persist(f) }
# ... later ...
kd2 = File.open("treefile") { |f| Kdtree.new(f) }
```

### Performance

Kdtree is fast. How fast? Using a tree with 1 million points on my i5 2.8ghz:

```
build (init)        3.52s
nearest point       0.000003s
nearest 5 points    0.000004s
nearest 50 points   0.000014s
nearest 255 points  0.000063s

persist             0.301963s
read (init)         0.432676s
```

### Limitations

* No **editing** allowed! Once you construct a tree you're stuck with it.
* The tree is stored in **one big memory block**, 20 bytes per point. A tree with one million points will allocate a single 19mb block to store its nodes.
* Persisted trees are **architecture dependent**, and may not work across different machines due to endian issues.
* nearestk is limited to **255 results**

### Contributors

Since this gem was originally released, several folks have contributed important patches:

* @antifuchs (thread safety)
* @evanphx (native cleanups, perf)
* @ghazel (C89 compliance)
* @mcerna (1.9 compat)

### Changelog

#### 0.3

* Ruby 1.9.x compatibility (@mcerna and others)
* renamed KDTree to the more idiomatic Kdtree
* use IO methods directly instead of rooting around in rb_io
* thread safe, no more statics (@antifuchs)
* C90 compliance, no warnings (@ghazel)
* native cleanups (@evanphx)

#### 0.2

skipped this version to prevent confusion with other flavors of the gem

#### 0.1

* Original release
