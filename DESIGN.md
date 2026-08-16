# DuraDB: Architecture & Design

DuraDB is a C++20 **HTAP** engine: one database for teams running **Postgres plus a
warehouse** who need OLTP and OLAP on the **same tables** without ETL, duplicate storage,
or stale analytics.

**In scope:** transactional apps with reporting, events with drilldown, high volume ingest
with live queries.

**Out of scope:** universal Postgres drop in; pure OLTP only; pure petabyte OLAP only.

---

## Problem

| Approach | Wins | Loses |
|----------|------|-------|
| Row store (OLTP) | Fast point writes | Slow column scans at scale |
| Column store (OLAP) | Fast aggregations | Awkward concurrent writes |
| Postgres + ETL → warehouse | Simple write path | Two systems, stale analytics |

DuraDB removes the **operational** split (two databases, ETL lag). It does not remove
**performance** physics: one engine is strong on both sides for target workloads, not
best in class OLTP and OLAP on every benchmark simultaneously.

---

## Pattern

**Dual path HTAP on one catalog.** Same logical table, two physical routes:

| Path | For | Mechanism |
|------|-----|-----------|
| **OLTP** | Point lookups, updates, small results | Indexes, memtable, MVCC, tombstone merge |
| **OLAP** | Scans, filters, aggregations | Column segments, vectorised batches, zone maps |

**Storage model:** append only columnar **row groups** (~64k rows) in immutable
**segments**, plus an **active memtable** for hot writes. Queries scan **memtable ∪
segments**. Flush is seconds or size bound, not ETL.

**Ingest model:** `COPY` / binary batches for bulk; SQL `INSERT` for transactional rows.

**Durability:** WAL at batch boundaries; group commit (~1 to 10 ms or N MB).

Proven shape: TiDB, SingleStore, SQL Server columnstore, DuckDB/ClickHouse column chunks
with transactional indexes where OLTP is required.

---

## Architecture

```
                    SQL / REPL / API
                           │
              Frontend → Binder → Executor
                  (vectorised + row ops)
                           │
         ┌─────────────────┼─────────────────┐
         ▼                 ▼                 ▼
    Bulk ingest      Query router       Catalog
    COPY / binary    OLTP ∪ OLAP        DDL, stats
         │                 │
         └────────┬────────┘
                  ▼
    ┌─────────────────────────────────────────┐
    │  Memtable (active RG) ──flush──► Segments │
    │       │ indexes / MVCC      immutable RGs │
    │       │                     + zone maps  │
    └───────┴─────────────────────────────────┘
                  │
                  ▼
           WAL → checkpoint
```

**Row group:** fixed width columns as arrays; `TEXT` as offset + bytes blob.
**Segment:** row groups + zone maps + catalog metadata.
**Index:** B tree on hot columns for the OLTP path.

---

## Principles

1. **Columnar by default.** Minimise I/O and enable SIMD; row heap is bootstrap only.
2. **Append only writes.** New data appends; updates/deletes via tombstones + merge.
3. **Immediate visibility.** Memtable queried on every read; no async replica.
4. **Bulk first.** Billion row path is batch API, not per row SQL.
5. **Vectorised execution.** ~2048 values per column per batch, not one row iterators.
6. **Zone maps before indexes.** Min/max per row group skips IO cheaply on ingest.
7. **MVCC.** Readers and writers concurrent; maturity grows incrementally toward Postgres parity.
8. **Route, don't unify.** Planner picks OLTP or OLAP path; one catalog, not two databases.

---

## Expectations

### v1 production target

| Area | Expectation |
|------|-------------|
| Deployment | One DB replaces Postgres + warehouse for target workloads |
| Transactions | ACID, core SQL, constraints, indexes on hot columns |
| OLTP | Competitive indexed point access; sub ms p99 as measured target |
| OLAP | Vectorised scans and aggregations on same tables |
| Freshness | New rows visible immediately; durable within seconds |
| Bulk load | Billion rows via batch API without starving concurrent OLTP |
| Recovery | Crash restart restores committed batches |

### Long term

Cost based optimiser, JOINs, broader Postgres semantics, distributed writes, extension
ecosystem. Not required for v1 credibility.

### Not v1

Every Postgres workload; beating dedicated OLAP on all benchmarks; triggers/exotic types;
multi region consensus.

---

## Success metrics

| Metric | Target |
|--------|--------|
| Bulk ingest | Saturate NVMe sequential bandwidth (batch path) |
| Point access (indexed) | Sub ms p99 at high concurrency |
| Analytical scan | IO ∝ matched row groups |
| Freshness | Immediate read; durable flush ≤ 5 s |
| OLTP under load | Usable concurrent queries during bulk ingest |
| Correctness | Tests pass at each stage |
