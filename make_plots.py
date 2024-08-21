# Plots for Speedup, Strong/Weak Scalability, Efficiency
import matplotlib.pyplot as plt
import pandas as pd
import os
import numpy as np


def cls(): os.system('cls')
def clear(): os.system('clear')

def get_dataframe(filename: str):
    return pd.read_csv(filename)

def select_speedup_data(
        df: pd.DataFrame, N: int, tileSize: int, policy: int,
        chunkSize: int = 1, drop_columns: bool = True
):
    filtered_parallel_df = df[
        (df['N'] == N) &
        (df['tileSize'] == tileSize) &
        (df['policy'] == policy) &
        (df['nworkers'] > 1)
    ]
    if policy == 3:
        filtered_parallel_df = filtered_parallel_df[filtered_parallel_df['chunkSize'] == chunkSize]
    filtered_sequential_df = df[
        (df['N'] == N) &
        (df['tileSize'] == 1) &
        (df['policy'] == 0)
    ]
    if drop_columns:
        if 'chunkSize' in filtered_parallel_df:
            filtered_parallel_df = filtered_parallel_df.drop(columns=['N', 'tileSize', 'policy', 'chunkSize'])
        else:
            filtered_parallel_df = filtered_parallel_df.drop(columns=['N', 'tileSize', 'policy'])
        if 'chunkSize' in filtered_sequential_df:
            filtered_sequential_df = filtered_sequential_df.drop(columns=['N', 'tileSize', 'policy', 'chunkSize'])
        else:
            filtered_sequential_df = filtered_sequential_df.drop(columns=['N', 'tileSize', 'policy'])
    speedup_df = pd.concat([filtered_sequential_df, filtered_parallel_df])
    sequential_time = filtered_sequential_df['time']
    speedup_df['speedup'] = speedup_df['time'].apply(lambda x : sequential_time / x)
    return speedup_df


def select_strong_scalability_data(
    df: pd.DataFrame, N: int, tileSize: int, policy: int,
    chunkSize: int = 1, drop_columns: bool = True
):
    if 'policy' in df:
        filtered_df = df[
            (df['N'] == N) &
            (df['tileSize'] == tileSize) &
            (df['policy'] == policy)
        ]
    else:
        filtered_df = df[
            (df['N'] == N) &
            (df['tileSize'] == tileSize)
        ]
    if policy == 3:
        filtered_df = filtered_df[filtered_df['chunkSize'] == chunkSize]
    if drop_columns:
        if 'policy' in df:
            if 'chunkSize' in filtered_df:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize', 'policy', 'chunkSize'])
            else:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize', 'policy'])
        else:
            if 'chunkSize' in filtered_df:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize', 'chunkSize'])
            else:
                filtered_df = filtered_df.drop(columns=['N', 'tileSize'])
    single_thread_time = filtered_df[filtered_df['nworkers'] == 1]['time']
    print(filtered_df, single_thread_time, sep='\n\n')
    filtered_df['strong_scalability'] = filtered_df['time'].apply(lambda x : single_thread_time / x)
    return filtered_df


def select_weak_scalability_data(
    df: pd.DataFrame, N: int, tileSize: int, policy: int,
    chunkSize: int = 1, maxN: int = 8000, drop_columns: bool = True
):
    filtered_df = df[
        (df['tileSize'] == tileSize) &
        (df['policy'] == policy)
    ]
    if policy == 3:
        filtered_df = filtered_df[filtered_df['chunkSize'] == chunkSize]
    if drop_columns:
        if 'chunkSize' in filtered_df:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy', 'chunkSize'])
        else:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy'])
    p = 2
    result_df = filtered_df[(filtered_df['N'] == N) & (filtered_df['nworkers'] == 1)]
    while (N * p <= maxN):
        newline = filtered_df[(filtered_df['N'] == p*N) & (filtered_df['nworkers'] == p)]
        if len(newline):
            result_df = pd.concat([result_df, newline])
        p += 1
    result_df['weak_scalability'] = result_df['time'] / result_df['nworkers']
    result_df['quadratic_weak_scalability'] = result_df['weak_scalability'] / result_df['nworkers']
    return result_df


def select_quadratic_weak_scalability_data(
    df: pd.DataFrame, N: int, tileSize: int, policy: int,
    chunkSize: int = 1, maxN: int = 8000, drop_columns: bool = True
):
    filtered_df = df[
        (df['tileSize'] == tileSize) &
        (df['policy'] == policy)
    ]
    if policy == 3:
        filtered_df = filtered_df[filtered_df['chunkSize'] == chunkSize]
    if drop_columns:
        if 'chunkSize' in filtered_df:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy', 'chunkSize'])
        else:
            filtered_df = filtered_df.drop(columns=['tileSize', 'policy'])
    p = 2
    result_df = filtered_df[(filtered_df['N'] == N) & (filtered_df['nworkers'] == 1)]
    while (N * p * p <= maxN):
        newline = filtered_df[(filtered_df['N'] == p*p*N) & (filtered_df['nworkers'] == p*p)]
        if len(newline):
            result_df = pd.concat([result_df, newline])
        p += 1
    result_df['weak_scalability'] = result_df['time'] / result_df['nworkers']
    return result_df


def select_efficiency_data(
        df: pd.DataFrame, N: int, tileSize: int, policy: int,
        chunkSize: int = 1, drop_columns: bool = True
):
    efficiency_df = select_speedup_data(df, N, tileSize, policy, chunkSize, drop_columns)
    efficiency_df['efficiency'] = efficiency_df['speedup'] / efficiency_df['nworkers'] * 100
    return efficiency_df

def round_to_significant_digits(value, digits):
    if value == 0:
        return 0
    return round(value, digits - int(np.floor(np.log10(abs(value)))) - 1)

def write_back_data(
    df: pd.DataFrame, base_filename: str,
    select_type: str, N: int, tileSize: int,
    policy: int, chunkSize: int = 1, digits: int = 4
):
    filename = f"{base_filename}_{select_type}_{N}_{policy}_{tileSize}_{chunkSize}.csv"
    for field in ['speedup', 'efficiency', 'strong_scalability_data', 'weak_scalability_data', 'time']:
        if field in df:
            df[field] = df[field].apply(lambda x: round_to_significant_digits(x, digits))
    df.to_csv(filename, index=False)
