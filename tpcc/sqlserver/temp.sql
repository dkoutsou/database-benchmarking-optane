SELECT name, physical_name AS CurrentLocation
FROM sys.master_files
WHERE database_id = DB_ID(N'tempdb');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev, FILENAME = '/path/to/data/');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev2, FILENAME = '/path/to/data/');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev3, FILENAME = '/path/to/data/');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev4, FILENAME = '/path/to/data/');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev5, FILENAME = '/path/to/data/');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev6, FILENAME = '/path/to/data/');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev7, FILENAME = '/path/to/data/');
GO

USE master;
GO
ALTER DATABASE tempdb
MODIFY FILE (NAME = tempdev8, FILENAME = '/path/to/data/');
GO

ALTER DATABASE tempdb
MODIFY FILE (NAME = templog, FILENAME = '/path/to/log/');
GO
