# Latency-Victims

This is the repo for the course project of CS683 (Advanced Computer Architecture).

## Commands to Run:
1. **Download the image**
   ```bash
    docker/podman pull docker.io/kanell21/artifact_evaluation:victima
   ```
2. **Compile the simluator**
   ```bash
   docker/podman run --rm -v $PWD:/app/ docker.io/kanell21/artifact_evaluation:victima /bin/bash -c "cd /app/sniper && make clean && make -j4"
   ```
4. **Extract Traces**
   ```bash
   tar -xzf traces.tar.gz
   ```

3. **To create a jobfile, run:**
    ```bash
    docker run --rm -v $PWD:/app/ docker.io/kanell21/artifact_evaluation:victima python /app/scripts/launch_jobs.py --slurm $PWD --excluded_nodes $3
    ```

4. **Submit the jobfile:**
    ```bash
    source jobfile
    ```

5. **To view the status of jobs:**
    ```bash
    squeue
    ```

6. **To cancel all the jobs:**
    ```bash
    scancel -u <username>
    ```

7. **Once all jobs are done, run this to create a CSV:**
    ```bash
    docker run --rm -v $PWD:/app/ docker.io/kanell21/artifact_evaluation:victima_ptwcp_v1.1 python3 /app/scripts/create_csv.py
    ```

8. **To create plots:**
    ```bash
    docker run --rm -v $PWD:/app/ docker.io/kanell21/artifact_evaluation:victima_ptwcp_v1.1 python3 /app/scripts/create_plots.py > plots_in_tabular.txt
    ```
We can do this altogether using this command as well: 
```bash
 sh artifact.sh --slurm docker
```
## Victima Codebase Modifications

### Files Updated in the Victima Codebase:

1. **Cache Folder**  
   - Updated files in:  
     `Victima/sniper/common/core/memory_subsystem/cache/`

2. **Cache Controller Files**  
   - Updated the following files in:  
     `Victima/sniper/common/core/memory_subsystem/parametric_dram_directory_msi/`
     - `cache_cntlr.cc`
     - `cache_cntlr.h`

3. **Plotting Script**  
   - Modified the `create_plots.py` file to generate the required plots.
   - The same plotting code was used to generate plots for Victima **without** the dead block predictor implemented.

---

### Experiments Conducted:

The experiments were run on the **`xs`** trace for **100,000,000 instructions**. Below are the experiments executed:

- `victima_ptw_1MBL2`
- `victima_ptw_2MBL2`
- `victima_ptw_4MBL2`
- `victima_ptw_8MBL2`
- `baseline_radix_1MB`
- `baseline_radix_2MB`
- `baseline_radix_4MB`
- `baseline_radix_8MB`
- `tlb_base_ideal`

---

## Steps to Reproduce Results:

1. **Replace the Cache Folder**  
   - Replace the folder at:  
     `Victima/sniper/common/core/memory_subsystem/cache/`  
     with the `cache` folder provided in this repository.

2. **Replace Cache Controller Files**  
   - Replace the following files at:  
     `Victima/sniper/common/core/memory_subsystem/parametric_dram_directory_msi/`
     - `cache_cntlr.cc`
     - `cache_cntlr.h`

3. **Replace Plotting Script**  
   - Replace the `create_plots.py` file at:  
     `Victima/scripts/create_plots.py`  
     with the `create_plots.py` file provided in this repository.

---

By following these steps, you can reproduce the results and generate the plots corresponding to the experiments mentioned above.


