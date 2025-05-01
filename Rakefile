require "bundler/gem_tasks"
require "rake/extensiontask"
require "rake/testtask"

Rake::ExtensionTask.new("kdtree")
task test: :compile

Rake::TestTask.new do
  _1.libs << "test"
  _1.test_files = FileList["test/**/test_*.rb"]
end
task default: :test
