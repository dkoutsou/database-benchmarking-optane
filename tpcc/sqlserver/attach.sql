CREATE DATABASE tpcc
    ON (FILENAME = '/path/to/tpcc.mdf'),
    (FILENAME = '/path/to/tpcc_log.ldf')
    FOR ATTACH;
GO
