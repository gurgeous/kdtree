Gem::Specification.new do |s|
  s.name        = "kdtree"
  s.version     = "0.2"

  s.authors     = ["Adam Doppelt"]
  s.email       = ["amd@gurge.com"]
  s.homepage    = "http://github.com/gurgeous/kdtree"
  s.summary     = "Blazingly fast, native 2d kdtree."
  s.description = <<EOF
A kdtree is a data structure that make it possible to quickly solve
the nearest neighbor problem. This is a native 2d kdtree suitable for
production use.
EOF

  s.rubyforge_project = "kdtree"

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.extensions = ["ext/extconf.rb"]
end
