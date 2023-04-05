USE master
GO

CREATE DATABASE [tpcc]
CONTAINMENT = NONE
ON  PRIMARY
( NAME = N'tpcc', FILENAME = N'/path/to/tpcc.mdf' ,
    SIZE = 100GB , FILEGROWTH = 1GB )
LOG ON
( NAME = N'tpcc_log', FILENAME = N'/path/to/tpcc_log.ldf' ,
    SIZE = 10GB , FILEGROWTH = 1GB )
GO

ALTER DATABASE [tpcc] SET RECOVERY SIMPLE
GO
