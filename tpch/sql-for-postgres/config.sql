alter system set wal_level='minimal';
alter system set archive_mode='off';
alter system set max_wal_senders=0;
alter system set max_wal_size='4GB';
alter system set maintenance_work_mem='128MB';
