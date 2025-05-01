require "bundler/setup"
require "rake/extensiontask"
require "rake/testtask"
require "bundler"
Bundler::GemHelper.install_tasks

#
# rake-compiler
#

Rake::ExtensionTask.new("kdtree")

#
# testing
#

Rake::TestTask.new do
  _1.libs << "test"
  _1.test_files = FileList["test/**/test_*.rb"]
end
task test: :compile
task default: :test
