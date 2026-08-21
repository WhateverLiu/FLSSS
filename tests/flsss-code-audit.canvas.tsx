import {
  Callout,
  Card,
  CardBody,
  CardHeader,
  Code,
  Divider,
  Grid,
  H1,
  H2,
  Pill,
  Row,
  Stack,
  Stat,
  Table,
  Text,
  useHostTheme,
} from "cursor/canvas";

const correctnessRows = [
  ["Critical", "Confirmed", "Variable-length heuristic drops real solutions", "flsss_variable_len.hpp:43–64", "A feasible candidate is discarded when the normal-tail estimate rounds to 0. Fixed len finds the solution; variable len does not."],
  ["Critical", "Confirmed", "Signed int64 sums overflow", "flsss_gen.hpp:78–92; flsss_core_detail.hpp:55–63", "The width check has no wider type beyond int64. UBSan confirms overflow for two INT64_MAX values."],
  ["High", "Confirmed", "Unsigned Val breaks complement bounds", "flsss_core_detail.hpp:70–79", "FLSSS_gen accepts unsigned integers, but total − upper-bound underflows. A valid 3-row uint8 case returns zero solutions."],
  ["High", "Confirmed", "Deadline is ignored inside contained-box enumeration", "flsss_solver.hpp:147–218, 299–311", "Time is checked only on node entry. One contained node emitted 705,432 results in 19 ms despite a 1 ms limit."],
  ["High", "Robustness", "TriMat allocation failure becomes null dereference", "trimat.hpp:35–58; flsss_solver.hpp:487", "build() silently returns after malloc failure; callers immediately access M[0]."],
  ["High", "Robustness", "Unchecked size products can under-allocate", "trimat.hpp:31–47; node_arena.hpp:111–128; flsss_solver.hpp:504–511", "Invalid or extreme dimensions can wrap value counts, arena sizes, and byte products before allocation."],
  ["High", "Direct API", "Ind can truncate nrow and len", "flsss_core_detail.hpp:134–158; flsss_solver.hpp:488–489", "FLSSS_core permits an index type too narrow for the row count; casts then silently corrupt geometry and indices."],
  ["High", "Direct API", "Unsigned Ind violates loop invariants", "findbound.hpp:42–45, 149–150; flsss_solver.hpp:232–233", "Several decrement-to-negative loops require a signed index type, but no static assertion enforces it."],
  ["Medium", "Confirmed", "Zero requested solutions can return one", "flsss_core_detail.hpp:210–229", "The fixed full-set complement shortcut returns the full set even when nSolutionsNeeded is 0."],
  ["Medium", "Contract", "maxIterations resets for every variable-length candidate", "flsss_variable_len.hpp:91–116; flsss_solver.hpp:483–513", "The deadline is shared, but each candidate receives a fresh full node budget."],
  ["Medium", "Contract", "Time limit excludes variable-length ranking", "flsss_variable_len.hpp:24–89", "The deadline is created only after moments, all candidate tightening/scoring, and sorting finish."],
  ["Medium", "Robustness", "Low-level helpers trust invalid dimensions and indices", "flsss_core_detail.hpp:16–49, 84–94; trimat.hpp:31–56", "Direct calls with len > nrow, invalid complement indices, or L > N can reach UB. Internal normal paths preserve these invariants."],
  ["Low", "Hardening", "Branch matrix lookup relies on an unstated invariant", "flsss_solver.hpp:220–259", "An immediate loop break would form row index −1. Current fuzzing did not reach it; add assertions before relying on it."],
  ["Low", "API", "Output tuple ordering is undocumented", "flsss_solver.hpp:183–188", "Indices are mapped through the leading-column permutation and are not guaranteed to be ascending."],
];

const optimizationRows = [
  ["1", "Very high", "Pre-sort every column once and build prefix sums", "Replace repeated strided gathers and two nth_element calls per column/candidate with O(1) k-smallest/k-largest sums after O(ncol·N log N) preprocessing."],
  ["2", "High", "Store only candidate metadata", "Keep k, flipped, probability, and leading column. Reconstruct tightened lo/hi into two reusable vectors at solve time; remove two heap vectors per candidate."],
  ["3", "High", "Cache leading-column permutations", "Candidates often share leadingC. Lazily cache rowOrder/leading column by column and reuse it instead of sorting X for every k."],
  ["4", "High", "Introduce a reusable solve workspace", "Reuse leading_col, verifyMatrix, verifyBounds, rowOrder, rootBounds, VerifyBand storage, solver vectors, and arena capacity across candidates."],
  ["5", "High", "Reuse TriMat capacity, not precomputed triangles", "Keep one sufficiently large allocation and rebuild values in place. This removes malloc/free churn without reviving the rejected all-k precomputation."],
  ["6", "High", "Reuse complement marking storage", "Allocate one epoch-mark array per batch instead of a fresh nrow-byte bitmap for every returned complemented solution."],
  ["7", "High", "Cap variable output reserve", "solutions.reserve(nSolutionsNeeded) can request enormous virtual memory. Mirror the solver's capped reserve policy."],
  ["8", "Medium", "Avoid zero-filling the arena reservation", "NodeArena::reserveBytes uses vector::resize, touching every reserved byte. Raw uninitialized storage can avoid this setup cost if lifetime/alignment are handled carefully."],
  ["9", "Medium", "Combine moments and column totals", "Variable mode scans X once for moments and again for totals. Accumulate both in one row-major pass and consolidate the three moment allocations."],
  ["10", "Medium", "Specialize VerifyBand for small ncol", "Compile-time small-column variants can place accumulators in inline storage and improve unrolling/vectorization; benchmark 2–8 columns."],
  ["11", "Benchmark", "Reduce child LB/UB copying", "Every child copies both full bound arrays. Copy only active len where safe, then evaluate copy-on-write only if profiles show this dominates."],
  ["12", "API option", "Offer streaming or flat result storage", "The vector<vector<Ind>> result shape inherently allocates once per solution. A callback or flat indices+offsets API can eliminate those allocations for high-volume consumers."],
];

const testRows = [
  ["Committed tests", "Pass", "All existing programs compiled and ran under ASan/UBSan."],
  ["Differential fuzz", "Pass", "3,000 exhaustive-oracle cases, 1–9 rows and 1–3 columns, fixed and variable lengths."],
  ["Tail-probability regression", "Fail", "[100, 1 × 99], target 100: fixed len returns 1; variable len returns 0."],
  ["int64 overflow probe", "Fail", "UBSan reports INT64_MAX + INT64_MAX in column_totals."],
  ["Unsigned complement probe", "Fail", "uint8 values [10,20,30], len 2, bounds [30,65]: expected 3; returned 0."],
  ["Deadline probe", "Fail", "22 choose 11 contained results: 705,432 emitted in 19 ms under a 1 ms limit."],
  ["Zero-request probe", "Fail", "Fixed len == nrow returns one solution when nSolutionsNeeded == 0."],
];

export default function FLSSSCodeAudit() {
  const theme = useHostTheme();
  return (
    <Stack gap={18} style={{ padding: 20, maxWidth: 1400, margin: "0 auto" }}>
      <Stack gap={6}>
        <Row align="center" justify="space-between" wrap>
          <H1>FLSSS code audit</H1>
          <Row gap={6} wrap>
            <Pill active>Read-only review</Pill>
            <Pill>C++20</Pill>
            <Pill>ASan + UBSan</Pill>
          </Row>
        </Row>
        <Text tone="secondary">
          Correctness, low-level safety, and temporary-allocation review. Findings distinguish supported-path failures from defensive hardening of directly callable internal templates.
        </Text>
      </Stack>

      <Grid columns={4} gap={12}>
        <Stat value="5" label="Confirmed result/contract failures" tone="danger" />
        <Stat value="3,000" label="Random oracle cases passed" tone="success" />
        <Stat value="14" label="Items flagged" tone="warning" />
        <Stat value="12" label="Optimization opportunities" tone="info" />
      </Grid>

      <Callout tone="danger" title="Fix first: heuristic probability must never be a feasibility test">
        <Text>
          In <Code>FLSSS_variable_len</Code>, keep candidates whose tightened bounds are feasible even when <Code>score.prob == 0</Code>. Zero should mean “rank last,” not “discard.”
        </Text>
      </Callout>

      <Stack gap={8}>
        <H2>Correctness and safety findings</H2>
        <Table
          headers={["Severity", "Reachability", "Finding", "Location", "Why it matters"]}
          rows={correctnessRows}
          rowTone={correctnessRows.map((r) =>
            r[0] === "Critical" ? "danger" : r[0] === "High" ? "warning" : r[0] === "Medium" ? "info" : "neutral"
          )}
          stickyHeader
          striped
          style={{ maxHeight: 620 }}
        />
      </Stack>

      <Divider />

      <Stack gap={8}>
        <H2>Allocation and performance opportunities</H2>
        <Text tone="secondary">
          The current <Code>col.clear()</Code> correctly reuses capacity across columns inside one tightening call. The expensive churn is across candidate calls and complete per-candidate solve setup.
        </Text>
        <Table
          headers={["Priority", "Expected impact", "Change", "Rationale"]}
          rows={optimizationRows}
          columnAlign={["center", "left", "left", "left"]}
          striped
        />
      </Stack>

      <Grid columns="1fr 1fr" gap={14}>
        <Card>
          <CardHeader trailing={<Pill size="sm" active>Recommended order</Pill>}>Implementation sequence</CardHeader>
          <CardBody>
            <Stack gap={8}>
              <Text><Code>1.</Code> Remove the probability feasibility gate and add its regression test.</Text>
              <Text><Code>2.</Code> Define numeric contracts: signed Val or widened sums; signed/range-checked Ind.</Text>
              <Text><Code>3.</Code> Enforce deadline inside contained enumeration and include preprocessing.</Text>
              <Text><Code>4.</Code> Harden TriMat sizing/allocation and cap output reservation.</Text>
              <Text><Code>5.</Code> Implement sorted-column prefix envelopes, then workspace reuse.</Text>
            </Stack>
          </CardBody>
        </Card>

        <Card>
          <CardHeader trailing={<Pill size="sm">Confidence</Pill>}>What looks sound</CardHeader>
          <CardBody>
            <Stack gap={8}>
              <Text>Small fixed and variable instances matched exhaustive brute force across 1–3 columns.</Text>
              <Text>The complement logic is correct for signed values on tested valid inputs.</Text>
              <Text>The iterative DFS and arena avoid per-node heap allocation.</Text>
              <Text>Row permutation and multi-column VerifyBand behavior passed the randomized oracle.</Text>
            </Stack>
          </CardBody>
        </Card>
      </Grid>

      <Stack gap={8}>
        <H2>Verification evidence</H2>
        <Table
          headers={["Check", "Result", "Evidence"]}
          rows={testRows}
          rowTone={testRows.map((r) => r[1] === "Pass" ? "success" : "danger")}
          striped
        />
      </Stack>

      <Callout tone="info" title="Test-suite gap">
        The committed tests are mostly one-dimensional ad-hoc executables; several only print or compile and cannot fail on a mismatch. Preserve the randomized small multi-column oracle as a real regression target.
      </Callout>

      <Text size="small" tone="tertiary" style={{ color: theme.text.tertiary }}>
        Review date: 2026-07-22. No production files were modified.
      </Text>
    </Stack>
  );
}
