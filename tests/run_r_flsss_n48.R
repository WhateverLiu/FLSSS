# Time CRAN FLSSS::FLSSS on the unique 1-D instance (n=48, k=14, seed=3).
# v, target are integers that sit exactly in IEEE-754 double (~2^48 and ~2^51).
# ME is a small band around the exact target (R has no int64 path).

v_raw <- scan("tests/instance_n48_k14_seed3.csv", quiet = TRUE)
meta <- readLines("tests/instance_n48_k14_seed3_meta.txt")
target <- as.numeric(sub("^target=", "", meta[grep("^target=", meta)]))
k <- as.integer(sub("^k=", "", meta[grep("^k=", meta)]))

stopifnot(length(v_raw) == 48L, k == 14L)
stopifnot(identical(v_raw, floor(v_raw)))
stopifnot(identical(target, floor(target)))

v <- sort(v_raw)
ME <- 0.5
tlimit <- 600

cat("n=", length(v), " k=", k, " target=", sprintf("%.0f", target),
    " ME=", ME, " solutionNeed=1 tlimit=", tlimit, "s\n", sep = "")
cat("FLSSS version ", as.character(packageVersion("FLSSS")), "\n", sep = "")

library(FLSSS)
gc()
t0 <- proc.time()
rst <- FLSSS::FLSSS(
  len = k,
  v = v,
  target = target,
  ME = ME,
  solutionNeed = 1L,
  tlimit = tlimit,
  NfractionDigits = Inf
)
elapsed <- proc.time() - t0

nsol <- length(rst)
cat("n_solutions=", nsol, "\n", sep = "")
if (nsol > 0L) {
  s <- sum(v[rst[[1L]]])
  cat("first_sol_sum=", sprintf("%.0f", s),
      " abs_err=", abs(s - target), "\n", sep = "")
  cat("first_sol_1based_sorted_idx=", paste(rst[[1L]], collapse = ","), "\n", sep = "")
}
cat("user_s=", elapsed[["user.self"]],
    " sys_s=", elapsed[["sys.self"]],
    " elapsed_s=", elapsed[["elapsed"]], "\n", sep = "")

out <- list(
  n = length(v), k = k, target = target, ME = ME,
  n_solutions = nsol, elapsed_s = unname(elapsed[["elapsed"]]),
  user_s = unname(elapsed[["user.self"]]),
  sys_s = unname(elapsed[["sys.self"]])
)
dput(out, "tests/run_r_flsss_n48_result.Rds.txt")
saveRDS(out, "tests/run_r_flsss_n48_result.rds")
