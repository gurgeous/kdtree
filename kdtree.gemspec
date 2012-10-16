Gem::Specification.new do |s|
  s.name        = "kdtree"
  s.version     = "0.3"

  s.authors     = ["Adam Doppelt"]
  s.email       = ["amd@gurge.com"]
  s.homepage    = "http://github.com/gurgeous/kdtree"
  s.summary     = "Blazingly fast, native 2d kdtree."
  s.description = <<EOF
A kdtree is a data structure that makes it possible to quickly solve
the nearest neighbor problem. This is a native 2d kdtree suitable for
production use with millions of points.
EOF

  s.rubyforge_project = "kdtree"
  s.add_development_dependency "rake-compiler"

  s.files      = `git ls-files`.split("\n")
  s.test_files = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.extensions = ["ext/kdtree/extconf.rb"]
  s.require_paths = ["lib"]
end
