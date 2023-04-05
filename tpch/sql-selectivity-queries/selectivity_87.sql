SELECT
    sum(l_extendedprice * l_discount) AS revenue
FROM
    lineitem
WHERE
    l_shipdate >= CAST('1993-01-01' AS date)
    AND l_shipdate < CAST('1999-01-01' AS date)
