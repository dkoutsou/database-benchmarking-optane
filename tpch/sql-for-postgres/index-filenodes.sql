select cl.relfilenode from pg_class cl
join pg_namespace nsp on cl.relnamespace = nsp.oid where nsp.nspname = 'public' and (relname like '%pkey' or relname like 'idx%');

