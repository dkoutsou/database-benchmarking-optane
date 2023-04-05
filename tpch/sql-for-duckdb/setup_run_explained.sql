SET memory_limit='16GB';
SET threads TO 48;
SET enable_progress_bar=false;
.timer on
.read queries_explained/q{number}.sql
