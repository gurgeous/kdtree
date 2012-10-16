require "rake"
require "rake/testtask"

spec = eval(File.read("kdtree.gemspec"))

#
# gem
#

task :build do
  system "gem build --quiet kdtree.gemspec"
end

task install: :build do
  system "sudo gem install --quiet kdtree-#{spec.version}.gem"
end

task release: :build do
  system "git tag -a #{spec.version} -m 'Tagging #{spec.version}'"
  system "git push --tags"
  system "gem push kdtree-#{spec.version}.gem"
end

#
# minitest
#

Rake::TestTask.new(:test) do |test|
  test.libs << "test"
end

task default: :test
