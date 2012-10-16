require "benchmark"
require "kdtree"
require "tempfile"
require "test/unit"

#
# create a tree
#

class KdtreeTest < Test::Unit::TestCase
  TMP = "#{Dir.tmpdir}/kdtree_test"

  def setup
    @points = (0...1000).map { |i| [rand_coord, rand_coord, i] }
    @kdtree = Kdtree.new(@points)
  end

  def teardown
    File.unlink(TMP) if File.exists?(TMP)
  end

  def test_nearest
    100.times do
      pt = [rand_coord, rand_coord]

      # kdtree search
      id = @kdtree.nearest(pt[0], pt[1])
      kdpt = @points[id]

      # slow search
      sortpt = @points.sort_by { |i| distance(i, pt) }.first

      # assert
      kdd = distance(kdpt, pt)
      sortd = distance(sortpt, pt)
      assert((kdd - sortd).abs < 0.0000001, "kdtree didn't return the closest result")
    end
  end

  def test_nearestk
    100.times do
      pt = [rand_coord, rand_coord]

      # kdtree search
      list = @kdtree.nearestk(pt[0], pt[1], 5)
      kdpt = @points[list.last]

      # slow search
      sortpt = @points.sort_by { |i| distance(i, pt) }[list.length - 1]

      # assert
      kdd = distance(kdpt, pt)
      sortd = distance(sortpt, pt)
      assert((kdd - sortd).abs < 0.0000001, "kdtree didn't return the closest result")
    end
  end

  def test_persist
    # write
    File.open(TMP, "w") { |f| @kdtree.persist(f) }
    # read
    kdtree2 = File.open(TMP, "r") { |f| Kdtree.new(f) }

    # now test some random points
    100.times do
      pt = [rand_coord, rand_coord]
      id1 = @kdtree.nearest(*pt)
      id2 = kdtree2.nearest(*pt)
      assert(id1 == id2, "kdtree2 differed from kdtree")
    end
  end

  def test_bad_magic
    File.open(TMP, "w") { |f| f.puts "That ain't right" }
    assert_raise RuntimeError do
      File.open(TMP, "r") { |f| Kdtree.new(f) }
    end
  end

  def test_eof
    File.open(TMP, "w") { |f| @kdtree.persist(f) }
    bytes = File.read(TMP)

    [2, 10, 100].each do |len|
      File.open(TMP, "w") { |f| f.write(bytes[0, len]) }
      assert_raise EOFError do
        File.open(TMP, "r") { |f| Kdtree.new(f) }
      end
    end
  end

  def dont_test_speed
    sizes = [1, 100, 1000, 10000, 100000, 1000000]
    ks = [1, 5, 50, 255]
    sizes.each do |s|
      points = (0...s).map { |i| [rand_coord, rand_coord, i] }

      # build
      Benchmark.bm(17) do |bm|
        kdtree = nil
        bm.report "build" do
          kdtree = Kdtree.new(points)
        end
        bm.report "persist" do
          File.open(TMP, "w") { |f| kdtree.persist(f) }
        end
        bm.report "read" do
          File.open(TMP, "r") { |f| Kdtree.new(f) }
        end

        ks.each do |k|
          bm.report "100 queries (#{k})" do
            total = count = 0
            100.times do
              tm = Time.now
              if k == 1
                kdtree.nearest(rand_coord, rand_coord)
              else
                kdtree.nearestk(rand_coord, rand_coord, k)
              end
            end
          end
        end
      end
      puts
    end
  end

  protected

  def distance(a, b)
    x, y = a[0] - b[0], a[1] - b[1]
    x * x + y * y
  end

  def rand_coord
    rand(0) * 10 - 5
  end
end

# running dont_test_speed on my i5 2.8ghz:
#
#                         user     system      total        real
# build               3.350000   0.020000   3.370000 (  3.520528)
# persist             0.150000   0.020000   0.170000 (  0.301963)
# read                0.280000   0.000000   0.280000 (  0.432676)
# 100 queries (1)     0.000000   0.000000   0.000000 (  0.000319)
# 100 queries (5)     0.000000   0.000000   0.000000 (  0.000412)
# 100 queries (50)    0.000000   0.000000   0.000000 (  0.001417)
# 100 queries (255)   0.000000   0.000000   0.000000 (  0.006268)
