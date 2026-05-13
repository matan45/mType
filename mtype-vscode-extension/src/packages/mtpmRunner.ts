import * as cp from 'child_process';
import * as readline from 'readline';
import * as vscode from 'vscode';

let inflight: Promise<number> | undefined;

export function isMtpmBusy(): boolean {
    return inflight !== undefined;
}

export function runMtpm(
    exe: string,
    args: string[],
    cwd: string,
    channel: vscode.OutputChannel
): Promise<number> {
    if (inflight) {
        const message = 'An mtpm operation is already running. Wait for it to finish.';
        void vscode.window.showWarningMessage(message);
        return Promise.reject(new Error(message));
    }

    const task = new Promise<number>((resolve, reject) => {
        channel.appendLine('');
        channel.appendLine(`> mtpm ${args.join(' ')}`);
        channel.appendLine(`  (cwd: ${cwd})`);

        let child: cp.ChildProcessWithoutNullStreams;
        try {
            child = cp.spawn(exe, args, { cwd, shell: false });
        } catch (err) {
            const msg = err instanceof Error ? err.message : String(err);
            channel.appendLine(`[mType PM] ERROR: failed to spawn mtpm: ${msg}`);
            reject(err);
            return;
        }

        const stdoutLines = readline.createInterface({ input: child.stdout });
        const stderrLines = readline.createInterface({ input: child.stderr });
        stdoutLines.on('line', (line) => channel.appendLine(`[mtpm] ${line}`));
        stderrLines.on('line', (line) => channel.appendLine(`[mtpm stderr] ${line}`));

        child.on('error', (err) => {
            channel.appendLine(`[mType PM] ERROR: ${err.message}`);
            reject(err);
        });

        child.on('close', (code) => {
            stdoutLines.close();
            stderrLines.close();
            const exitCode = code ?? -1;
            channel.appendLine(`[mType PM] mtpm exited with code ${exitCode}`);
            resolve(exitCode);
        });
    });

    inflight = task;
    const clear = () => {
        if (inflight === task) {
            inflight = undefined;
        }
    };
    task.then(clear, clear);
    return task;
}
