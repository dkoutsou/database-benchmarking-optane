-- set numa node to 0
ALTER SERVER CONFIGURATION
SET PROCESS AFFINITY NUMANODE=0
GO


-- see CPUs used by SQLserver

DECLARE @OnlineCpuCount int
DECLARE @HiddenCpuCount int
DECLARE @LogicalCpuCount int

SELECT @OnlineCpuCount = COUNT(*) FROM sys.dm_os_schedulers WHERE status = 'VISIBLE ONLINE'
SELECT @HiddenCpuCount = COUNT(*) FROM sys.dm_os_schedulers WHERE status != 'VISIBLE ONLINE'
SELECT @LogicalCpuCount = cpu_count FROM sys.dm_os_sys_info

SELECT @LogicalCpuCount AS 'ASSIGNED ONLINE CPU #', @OnlineCpuCount AS 'VISIBLE ONLINE CPU #', @HiddenCpuCount AS 'HIDDEN ONLINE CPU #',
   CASE
         WHEN @OnlineCpuCount < @LogicalCpuCount
                 THEN 'You are not using all CPU assigned to O/S! If it is VM, review your VM configuration to make sure you are not maxout Socket'
                     ELSE 'You are using all CPUs assigned to O/S. GOOD!'
                           END as 'CPU Usage Desc'
                        ----------------------------------------------------------------------------------------------------------------
                        GO
-- check if BPE is enabled
SELECT [path], state_description, current_size_in_kb,
CAST(current_size_in_kb/1048576.0 AS DECIMAL(10,2)) AS [Size (GB)]
FROM sys.dm_os_buffer_pool_extension_configuration;

GO

-- check BP per database
SELECT
  database_id AS DatabaseID,
  DB_NAME(database_id) AS DatabaseName,
  COUNT(file_id) * 8/1024.0 AS BufferSizeInMB
FROM sys.dm_os_buffer_descriptors
GROUP BY DB_NAME(database_id),database_id
ORDER BY BufferSizeInMB DESC
GO

--Reduce SQL Server Max memory to restrict the BP to 48GB
EXEC sys.sp_configure 'show advanced options', '1'  RECONFIGURE WITH OVERRIDE;
GO
EXEC sys.sp_configure 'max server memory (MB)', '49152';
GO
-- Set checkpoint to
EXEC sys.sp_configure 'recovery interval','1'
GO
RECONFIGURE WITH OVERRIDE;
GO
EXEC sys.sp_configure 'show advanced options', '0'  RECONFIGURE WITH OVERRIDE;
GO
