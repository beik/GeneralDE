# $(call assert,condition,message)
define assert
  $(if $1,,$(error Assertion failed: $2))
endef

# $(call assert-file-exists,wildcad-pattern)
define assert-file-exists
  $(call assert,$(wildcard $1),$1 does not exit)
endef

# $(call assert-not-null,make-variable)
define assert-not-null
  $(call assert,$($1),The variable "$1" is null)
endef

# $(call assert-set-one,make-variable-list)
define assert-set-one
  $(if $(filter 1,$(words $(foreach m,$1,$($m)))),,$(warning $1 only can set 1 value))
endef

# $(call join-path,part1,part2)
join-path=$(if $(1),$(strip $(1))/$(strip $(2)),$(2))

ifneq ($(V),1)
  define with_message
    @$(if $1,echo ">>> $1" &&, )
  endef

  CPE_SILENCE_TAG:=@
endif

ifeq ($(MKD),1)
  debug-warning=$(warning $1)
  debug-variables=$(warning $1)
endif

# $(call stack-push,stack-variable-name,word)
define stack-push
$1 := $2 $($1)
endef

# $(call stack-pop,stack-variable-name)
define stack-pop
$1 := $(wordlist 2,$(words $($1)),$($1))
endef

# $(call build-regular-path,output-variable,input-variable)
define build-regular-path
$(foreach part,$(subst /,$(space),$2),                          \
    $(if $(filter .,$(part)),,                                  \
    $(if $(filter ..,$(part)),$(eval $(call stack-pop,$1.inbuild))      \
                             ,$(eval $(call stack-push,$1.inbuild,$(part)))))   \
)
$(foreach result-part,$($1.inbuild),$(eval $1:=/$(result-part)$($1)))
endef

# $(call path-list-join,path-list)
define path-list-join
$(if $1,$(word 1,$1):$(call path-list-join,$(wordlist 2,$(words $1), $1)),)
endef

# $(call append-if-need,list,var)
append-if-need=$(if $(filter $(strip $2),$1),$1,$1 $(strip $2))

# $(call apend-force-to-end,list,var)
append-force-to-end=$(filter-out $(strip $2),$1) $(strip $2)

# $(call merge-list,list,list2)
merge-list=$(if $2,$(call append-if-need,$(call merge-list,$1,$(wordlist 2,$(words $2),$2)),$(word 1,$2)),$1)

# $(call merge-list,list,list2)
merge-force-to-end=$(filter-out $2, $1) $2

# $(call revert-list,$1)
revert-list=$(if $1,$(call revert-list,$(wordlist 2,$(words $1),$1)) $(word 1,$1))

# $(call regular-list,$1)
regular-list=$(if $1,$(word 1,$1) $(call regular-list,$(wordlist 2,$(words $1),$1)))

# $(call select-var,var-list)
define select-var
$(if $1\
     , $(warning $(word 1,$1))\
	   $(if $($(word 1, $1)) \
          , $($(word 1, $1))\
          , $(call select-var,$(wordlist 2,$(words $1), $1))) \
     ,)
endef

compiler-category=$(strip \
                      $(if $(filter %clang,$1),clang,\
                      $(if $(filter %clang++,$1),clang,\
                      $(if $(filter %gcc,$1),gcc,\
                      $(if $(filter %g++,$1),gcc,\
                      $(warning unknown compiler $1))))))

