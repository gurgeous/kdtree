require "bundler/setup"
require "rake/extensiontask"
require "rake/testtask"

# load the spec, we use it below
spec = Gem::Specification.load("kdtree.gemspec")

#
# gem
#

task :build do
  system "gem build --quiet kdtree.gemspec"
end

task :install => :build do
  system "sudo gem install --quiet kdtree-#{spec.version}.gem"
end

task :release => :build do
  system "git tag -a #{spec.version} -m 'Tagging #{spec.version}'"
  system "git push --tags"
  system "gem push kdtree-#{spec.version}.gem"
end

#
# rake-compiler
#

Rake::ExtensionTask.new("kdtree", spec)


#
# testing
#

Rake::TestTask.new(:test) do |test|
  test.libs << "test"
end
task :test => :compile
task :default => :test
