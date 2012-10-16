Gem::Specification.new do |s|
  s.name        = "kdtree"
  s.version     = "0.2"

  s.authors     = ["Adam Doppelt"]
  s.email       = ["amd@gurge.com"]
  s.homepage    = "http://github.com/gurgeous/kdtree"
  s.summary     = "Blazingly fast, native 2d kdtree."
  s.description = "Blazingly fast, native 2d kdtree."

  s.rubyforge_project = "kdtree"

  s.add_development_dependency("awesome_print")
  s.add_development_dependency("rake-compiler")

  s.files = %w[
    LICENSE
    Gemfile
    Gemfile.lock
    README.md
    Rakefile
    ext/kdtree/extconf.rb
    ext/kdtree/kdtree.c
    lib/kdtree.rb
    test/test_kdtree.rb
  ]
  s.test_files = ["test/test_kdtree.rb"]
  s.extensions = ["ext/kdtree/extconf.rb"]
  s.require_paths = ["lib"]
end
