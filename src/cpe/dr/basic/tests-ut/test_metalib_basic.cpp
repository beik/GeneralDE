#include "gtest/gtest.h"
#include "cpe/dr/dr_metalib_init.h"
#include "cpe/dr/dr_metalib_manage.h"
#include "with_InputMetaLibTest.hpp"

class MetalibManagerTest : public ::testing::Test, public WithInputMetaLibTest {
    virtual void SetUp() {
        loadLib();
    }

    virtual void TearDown() {
        freeLib();
    }
};

TEST_F(MetalibManagerTest, Size) {
    ASSERT_EQ((size_t)m_libSize, dr_lib_size(m_lib));
}

TEST_F(MetalibManagerTest, id) {
    ASSERT_EQ(123, m_lib->m_id);
}

TEST_F(MetalibManagerTest, Name) {
    ASSERT_STREQ("net", dr_lib_name(m_lib));
}

TEST_F(MetalibManagerTest, Version) {
    ASSERT_EQ(10, dr_lib_version(m_lib));
}

TEST_F(MetalibManagerTest, BuildVersion) {
    ASSERT_EQ(11, dr_lib_build_version(m_lib));
}

TEST_F(MetalibManagerTest, GetMicroNum) {
    ASSERT_EQ(7, dr_lib_macro_num(m_lib));
}

TEST_F(MetalibManagerTest, GetMicroByIdx_Overflow) {
    ASSERT_TRUE(dr_lib_macro_at(m_lib, -1) == 0);
    ASSERT_TRUE(dr_lib_macro_at(m_lib, 7) == 0);
}

TEST_F(MetalibManagerTest, GetMicroValueExist) {
    int checkValue = -1;
    ASSERT_EQ(0, dr_lib_find_macro_value(&checkValue, m_lib, "VERSION"));

    ASSERT_EQ(100, checkValue);
}

TEST_F(MetalibManagerTest, GetMicroValueNotExist) {
    int checkValue = -1;
    ASSERT_LT(dr_lib_find_macro_value(&checkValue, m_lib, "NotExistMacro"), 0);

    ASSERT_EQ(-1, checkValue);
}

TEST_F(MetalibManagerTest, GetMetaByNameExist) {
    LPDRMETA lpMeta = dr_lib_find_meta_by_name(m_lib, "PkgBody");
    ASSERT_TRUE(lpMeta != 0) << "dr_find_meta_by_name(PkgBody) fail";

    ASSERT_STREQ("PkgBody", dr_meta_name(lpMeta));
}

TEST_F(MetalibManagerTest, GetMetaByNameNotExist) {
    ASSERT_EQ(NULL, dr_lib_find_meta_by_name(m_lib, "NotExistEntry")) << "dr_find_meta_by_name(NotExistEntry) fail";
}

TEST_F(MetalibManagerTest, GetMetaByIdExist) {
    LPDRMETA lpMeta = dr_lib_find_meta_by_id(m_lib, 1);
    ASSERT_TRUE(lpMeta != 0) << "dr_get_meta_by_id(1) fail";

    ASSERT_STREQ("PkgHead", dr_meta_name(lpMeta));
}

TEST_F(MetalibManagerTest, GetMetaByIdNotExist) {
    ASSERT_EQ(NULL, dr_lib_find_meta_by_id(m_lib, 34)) << "dr_get_meta_by_id(34) should not exist";
}

TEST_F(MetalibManagerTest, GetMetaByIdNegative) {
    ASSERT_EQ(NULL, dr_lib_find_meta_by_id(m_lib, -1)) << "dr_get_meta_by_id(-1) should not exist";
}


TEST_F(MetalibManagerTest, MetaOrder) {
    EXPECT_EQ(5, dr_lib_meta_num(m_lib));
    EXPECT_STREQ("PkgHead", dr_meta_name(dr_lib_meta_at(m_lib, 0)));
    EXPECT_STREQ("CmdLogin", dr_meta_name(dr_lib_meta_at(m_lib, 1)));
    EXPECT_STREQ("CmdLogout", dr_meta_name(dr_lib_meta_at(m_lib, 2)));
}

TEST_F(MetalibManagerTest, MetaKey) {
    EXPECT_EQ(0, dr_meta_key_entry_num(meta("PkgHead")));

    LPDRMETA m = meta("CmdLogin");
    EXPECT_EQ(2, dr_meta_key_entry_num(m));

    EXPECT_STREQ("name", dr_entry_name(dr_meta_key_entry_at(m, 0)));
    EXPECT_STREQ("pass", dr_entry_name(dr_meta_key_entry_at(m, 1)));
}

TEST_F(MetalibManagerTest, MetaIndex) {
    EXPECT_EQ(0, dr_meta_key_entry_num(meta("PkgHead")));

    LPDRMETA m = meta("CmdLogin");

    dr_index_info_t i1 = dr_meta_index_at(m, 0);
    ASSERT_TRUE(i1);
    EXPECT_STREQ("index1", dr_index_name(i1));
    EXPECT_EQ(2, dr_index_entry_num(i1));

    dr_index_entry_info_t ie1 = dr_index_entry_info_at(i1, 0);
    ASSERT_TRUE(ie1);

    dr_index_entry_info_t ie2 = dr_index_entry_info_at(i1, 1);
    ASSERT_TRUE(ie2);

    EXPECT_STREQ("name", dr_entry_name(dr_index_entry_at(i1, 0)));
    EXPECT_STREQ("pass", dr_entry_name(dr_index_entry_at(i1, 1)));


    dr_index_info_t i2 = dr_meta_index_at(m, 1);
    ASSERT_TRUE(i2);
    EXPECT_STREQ("index2", dr_index_name(i2));
    EXPECT_EQ(1, dr_index_entry_num(i2));

    EXPECT_TRUE(NULL == dr_meta_index_at(m, 2));
}
