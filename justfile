default: test

#
# ci/test
#

# check repo - lint & test
check: lint test

# for ci. don't bother linting on windows
ci:
  @just test

# run tests
test *ARGS:
  @just _banner rake test {{ARGS}}
  @bundle exec rake test {{ARGS}}

#
# build/release
#

clean:
  rm -rf pkg tmp lib/*.bundle lib/*.so

gem-build: check clean
  @just _banner rake build...
  @bundle exec rake build

# this will tag, build and push to rubygems
gem-push: check clean
  @just _banner rake release...
  rake release

#
# lint
#

# format with rubocop
format: (lint "-a")

# lint with rubocop
lint *ARGS:
  @just _banner lint...
  bundle exec rubocop {{ARGS}}


#
# util
#

_banner *ARGS: (_message BG_GREEN ARGS)
_warning *ARGS: (_message BG_YELLOW ARGS)
_fatal *ARGS: (_message BG_RED ARGS)
  @exit 1
_message color *ARGS:
  @msg=$(printf "[%s] %s" $(date +%H:%M:%S) "{{ARGS}}") ; \
  printf "{{color+BOLD+WHITE}}%-72s{{ NORMAL }}\n" "$msg"
