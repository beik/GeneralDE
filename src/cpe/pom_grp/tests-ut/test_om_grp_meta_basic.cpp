#include "OmGrpMetaTest.hpp" 

TEST_F(OmGrpMetaTest, normal_basic) {
    install(
        "TestObj:\n"
        "  - entry1: { entry-type: normal, data-type: AttrGroup1 }\n"
        ,
        "<metalib tagsetversion='1' name='net'  version='1'>"
        "    <struct name='AttrGroup1' version='1'>"
        "	     <entry name='a1' type='uint32' id='1'/>"
        "    </struct>"
        "</metalib>"
        ) ;


    EXPECT_STREQ(
        "pom_grp_meta: name=TestObj, page-size=256, class-id=1, obj-size=4, page-count=1, size-buf-start=4, size-buf-count=0\n"
        "    entry1: entry-type=normal, data=type=AttrGroup1, page-begin=0, page-count=1, class-id=2, obj-size=4, obj-align=1"
        ,
        t_pom_grp_meta_dump(m_meta));
}

TEST_F(OmGrpMetaTest, list_basic) {
    install(
        "TestObj:\n"
        "  - entry1: { entry-type: list, data-type: AttrGroup1, group-count: 3, capacity: 3 }\n"
        ,
        "<metalib tagsetversion='1' name='net'  version='1'>"
        "    <struct name='AttrGroup1' version='1'>"
        "	     <entry name='a1' type='uint32' id='1'/>"
        "    </struct>"
        "</metalib>"
        ) ;


    EXPECT_STREQ(
        "pom_grp_meta: name=TestObj, page-size=256, class-id=1, obj-size=6, page-count=1, size-buf-start=4, size-buf-count=1\n"
        "    entry1: entry-type=list, data=type=AttrGroup1, capacity=3, size-idx=0, page-begin=0, page-count=1, class-id=2, obj-size=12, obj-align=1"
        ,
        t_pom_grp_meta_dump(m_meta));
}

TEST_F(OmGrpMetaTest, bitarry_basic) {
    t_em_set_print();
    install(
        "TestObj:\n"
        "  - entry1: { entry-type: ba, bit-capacity: 15 }\n"
        ,
        "<metalib tagsetversion='1' name='net'  version='1'>"
        "</metalib>"
        ) ;

    EXPECT_STREQ(
        "pom_grp_meta: name=TestObj, page-size=256, class-id=1, obj-size=4, page-count=1, size-buf-start=4, size-buf-count=0\n"
        "    entry1: entry-type=ba, bit-capacity=15, page-begin=0, page-count=1, class-id=2, obj-size=2, obj-align=1"
        ,
        t_pom_grp_meta_dump(m_meta));
}

TEST_F(OmGrpMetaTest, binary_basic) {
    install(
        "TestObj:\n"
        "  - entry1: { entry-type: binary, capacity: 7 }\n"
        ,
        "<metalib tagsetversion='1' name='net'  version='1'>"
        "</metalib>"
        ) ;


    EXPECT_STREQ(
        "pom_grp_meta: name=TestObj, page-size=256, class-id=1, obj-size=4, page-count=1, size-buf-start=4, size-buf-count=0\n"
        "    entry1: entry-type=binary, capacity=7, page-begin=0, page-count=1, class-id=2, obj-size=7, obj-align=1"
        ,
        t_pom_grp_meta_dump(m_meta));
}

TEST_F(OmGrpMetaTest, basic_multi) {
    install(
        "TestObj:\n"
        "  - entry1: { entry-type: normal, data-type: AttrGroup1 }\n"
        "  - entry2: { entry-type: list, data-type: AttrGroup2, group-count: 3, capacity: 3 }\n"
        "  - entry3: { entry-type: binary, capacity: 5 }\n"
        ,
        "<metalib tagsetversion='1' name='net'  version='1'>"
        "    <struct name='AttrGroup1' version='1'>"
        "	     <entry name='a1' type='uint32' id='1'/>"
        "    </struct>"
        "    <struct name='AttrGroup2' version='1'>"
        "	     <entry name='b1' type='string' size='5' id='1'/>"
        "    </struct>"
        "</metalib>"
        ) ;


    EXPECT_STREQ(
        "pom_grp_meta: name=TestObj, page-size=256, class-id=1, obj-size=14, page-count=3, size-buf-start=12, size-buf-count=1\n"
        "    entry1: entry-type=normal, data=type=AttrGroup1, page-begin=0, page-count=1, class-id=2, obj-size=4, obj-align=1\n"
        "    entry2: entry-type=list, data=type=AttrGroup2, capacity=3, size-idx=0, page-begin=1, page-count=1, class-id=3, obj-size=15, obj-align=1\n"
        "    entry3: entry-type=binary, capacity=5, page-begin=2, page-count=1, class-id=4, obj-size=5, obj-align=1"
        ,
        t_pom_grp_meta_dump(m_meta));
}