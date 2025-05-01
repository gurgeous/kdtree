Gem::Specification.new do |s|
  s.name = "kdtree"
  s.version = "0.5"
  s.authors = ["Adam Doppelt"]
  s.email = "amd@gurge.com"

  s.summary = "Blazingly fast, native 2d kdtree."
  s.homepage = "http://github.com/gurgeous/kdtree"
  s.description = <<~EOF
    A kdtree is a data structure that makes it possible to quickly solve
    the nearest neighbor problem. This is a native 2d kdtree suitable for
    production use with millions of points.
  EOF
  s.license = "MIT"
  s.required_ruby_version = ">= 3.0.0"
  s.metadata = {
    "homepage_uri" => s.homepage,
    "rubygems_mfa_required" => "true",
    "source_code_uri" => s.homepage,
  }

  s.files = `git ls-files`.split("\n")
  s.extensions = ["ext/kdtree/extconf.rb"]
  s.require_paths = ["lib"]
end
