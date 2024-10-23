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
