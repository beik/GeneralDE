product:=gtest
$(product).type:=lib
$(product).buildfor:=dev
$(product).product.c.includes:=3rdTools/gtest/include
$(product).c.sources:=$(wildcard $(product-base)/*.cc)
$(product).c.includes:=3rdTools/gtest
$(product).product.c.libraries:=pthread

$(eval $(call product-def,$(product)))
