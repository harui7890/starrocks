// This file is licensed under the Elastic License 2.0. Copyright 2021-present, StarRocks Limited.

package com.starrocks.sql.analyzer;

import com.starrocks.catalog.Database;
import com.starrocks.catalog.Table;
import com.starrocks.sql.ast.CreateAnalyzeJobStmt;
import com.starrocks.statistic.AnalyzeJob;
import com.starrocks.utframe.StarRocksAssert;
import com.starrocks.utframe.UtFrameUtils;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;

import static com.starrocks.sql.analyzer.AnalyzeTestUtil.analyzeSuccess;
import static com.starrocks.sql.analyzer.AnalyzeTestUtil.getStarRocksAssert;

public class AnalyzeCreateAnalyzeJobTest {
    private static StarRocksAssert starRocksAssert;

    @BeforeClass
    public static void beforeClass() throws Exception {
        UtFrameUtils.createMinStarRocksCluster();
        AnalyzeTestUtil.init();
        starRocksAssert = getStarRocksAssert();

        String createTblStmtStr = "create table db.tbl(kk1 int, kk2 varchar(32), kk3 int, kk4 int) "
                + "AGGREGATE KEY(kk1, kk2,kk3,kk4) distributed by hash(kk1) buckets 3 properties('replication_num' = "
                + "'1');";
        starRocksAssert = new StarRocksAssert();
        starRocksAssert.withDatabase("db").useDatabase("db");
        starRocksAssert.withTable(createTblStmtStr);
    }

    @Test
    public void testAllDB() throws Exception {
        String sql = "create analyze all";
        CreateAnalyzeJobStmt analyzeStmt = (CreateAnalyzeJobStmt) analyzeSuccess(sql);

        Assert.assertEquals(AnalyzeJob.DEFAULT_ALL_ID, analyzeStmt.getDbId());
        Assert.assertEquals(AnalyzeJob.DEFAULT_ALL_ID, analyzeStmt.getTableId());
        Assert.assertTrue(analyzeStmt.getColumnNames().isEmpty());
    }

    @Test
    public void testAllTable() throws Exception {
        String sql = "create analyze full database db";
        CreateAnalyzeJobStmt analyzeStmt = (CreateAnalyzeJobStmt) analyzeSuccess(sql);

        Database db = starRocksAssert.getCtx().getGlobalStateMgr().getDb("default_cluster:db");
        Assert.assertEquals(db.getId(), analyzeStmt.getDbId());
        Assert.assertEquals(AnalyzeJob.DEFAULT_ALL_ID, analyzeStmt.getTableId());
        Assert.assertTrue(analyzeStmt.getColumnNames().isEmpty());
    }

    @Test
    public void testColumn() throws Exception {
        String sql = "create analyze table db.tbl(kk1, kk2)";
        CreateAnalyzeJobStmt analyzeStmt = (CreateAnalyzeJobStmt) analyzeSuccess(sql);

        Database db = starRocksAssert.getCtx().getGlobalStateMgr().getDb("default_cluster:db");
        Assert.assertEquals(db.getId(), analyzeStmt.getDbId());
        Table table = db.getTable("tbl");
        Assert.assertEquals(table.getId(), analyzeStmt.getTableId());
        Assert.assertEquals(2, analyzeStmt.getColumnNames().size());
    }
}
