import os
import sys

# Ensure dependencies are available
try:
    import pandas as pd
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("[ERROR] Missing python libraries. Please run: pip install pandas matplotlib numpy")
    sys.exit(1)

def main():
    # Find all measurement CSV files in the current directory
    csv_files = [f for f in os.listdir('.') if f.startswith('measurement_results') and f.endswith('.csv')]
    
    if not csv_files:
        print("[ERROR] No measurement results CSV files found (e.g. measurement_results_s11_18_26.5ghz.csv).")
        print("Please run 'visa_test_msvc.exe' first to capture measurement data.")
        return
        
    if len(csv_files) == 1:
        csv_filename = csv_files[0]
    else:
        print("Multiple measurement results files found:")
        for idx, f in enumerate(csv_files):
            print(f"  {idx + 1}. {f}")
        choice = input(f"Select file to plot [1-{len(csv_files)}, default: 1]: ")
        try:
            choice_idx = int(choice) - 1 if choice.strip() else 0
            if 0 <= choice_idx < len(csv_files):
                csv_filename = csv_files[choice_idx]
            else:
                csv_filename = csv_files[0]
        except ValueError:
            csv_filename = csv_files[0]

    print(f"[INFO] Reading measurement data from {csv_filename}...")
    try:
        df = pd.read_csv(csv_filename)
    except Exception as e:
        print(f"[ERROR] Failed to read CSV file: {e}")
        return

    # Check columns
    has_angle = 'Angle_deg' in df.columns
    required_cols = {'Frequency_Hz', 'Magnitude_dB', 'Phase_deg'}
    if not required_cols.issubset(df.columns):
        print(f"[ERROR] CSV file missing required columns. Expected at least: {required_cols}")
        return

    base_name = os.path.splitext(csv_filename)[0]

    if not has_angle:
        # ------------------------------------------------------------------
        # MODE 1: Static frequency sweep (S11 magnitude/phase vs frequency)
        # ------------------------------------------------------------------
        print("[INFO] Detected STATIC frequency sweep data.")
        df_sorted = df.sort_values(by='Frequency_Hz')
        freq_ghz = df_sorted['Frequency_Hz'] / 1e9
        mag_db = df_sorted['Magnitude_dB']
        phase_deg = df_sorted['Phase_deg']

        # --- Magnitude Plot ---
        plt.figure(figsize=(10, 5))
        plt.plot(freq_ghz, mag_db, color='dodgerblue', linewidth=2)
        plt.title(f'S11 Magnitude vs Frequency ({base_name})')
        plt.xlabel('Frequency (GHz)')
        plt.ylabel('S11 Magnitude (dB)')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        mag_img = f'{base_name}_magnitude_plot.png'
        plt.savefig(mag_img, dpi=300)
        print(f"[SUCCESS] Magnitude plot saved as: {mag_img}")

        # --- Phase Plot ---
        plt.figure(figsize=(10, 5))
        plt.plot(freq_ghz, phase_deg, color='darkorange', linewidth=2)
        plt.title(f'S11 Phase vs Frequency ({base_name})')
        plt.xlabel('Frequency (GHz)')
        plt.ylabel('S11 Phase (degrees)')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        phase_img = f'{base_name}_phase_plot.png'
        plt.savefig(phase_img, dpi=300)
        print(f"[SUCCESS] Phase plot saved as: {phase_img}")

        plt.show()

    else:
        # ------------------------------------------------------------------
        # MODE 2: Coordinated radiation pattern sweep (Frequency + Angle)
        # ------------------------------------------------------------------
        print("[INFO] Detected COORDINATED angular sweep data.")
        angles = sorted(df['Angle_deg'].unique())
        freqs = sorted(df['Frequency_Hz'].unique())
        print(f"[INFO] Found {len(angles)} unique angles and {len(freqs)} frequency points.")

        print("\nChoose Plot Type:")
        print("  1. Standard Cartesian Curves (Magnitude vs Frequency for each Angle)")
        print("  2. 2D Polar Radiation Pattern (Magnitude vs Angle at a specific Frequency)")
        plot_choice = input("Enter choice [1-2, default: 2]: ")
        if not plot_choice.strip():
            plot_choice = "2"

        if plot_choice == "1":
            # --- Cartesian Curves ---
            plt.figure(figsize=(10, 6))
            for angle in angles:
                sub_df = df[df['Angle_deg'] == angle].sort_values(by='Frequency_Hz')
                freq_ghz = sub_df['Frequency_Hz'] / 1e9
                mag_db = sub_df['Magnitude_dB']
                plt.plot(freq_ghz, mag_db, label=f'{angle:.1f}°')
            
            plt.title(f'Magnitude vs Frequency ({base_name})')
            plt.xlabel('Frequency (GHz)')
            plt.ylabel('Magnitude (dB)')
            plt.grid(True, linestyle='--', alpha=0.7)
            plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
            plt.tight_layout()
            cart_img = f'{base_name}_cartesian_plot.png'
            plt.savefig(cart_img, dpi=300)
            print(f"[SUCCESS] Cartesian plot saved as: {cart_img}")
            plt.show()

        else:
            # --- Polar Radiation Pattern ---
            print("\nSelect a Frequency for the polar plot:")
            # Display a subset of available frequencies
            display_count = min(10, len(freqs))
            indices = np.linspace(0, len(freqs) - 1, display_count, dtype=int)
            for idx in indices:
                print(f"  {freqs[idx] / 1e9:.4f} GHz")
            
            default_freq = freqs[len(freqs) // 2]
            freq_str = input(f"Enter target frequency in GHz [default: {default_freq / 1e9:.4f}]: ")
            
            if not freq_str.strip():
                target_hz = default_freq
            else:
                try:
                    target_hz = float(freq_str) * 1e9
                except ValueError:
                    target_hz = default_freq

            # Find nearest frequency
            nearest_freq = min(freqs, key=lambda x: abs(x - target_hz))
            print(f"[INFO] Using nearest frequency: {nearest_freq / 1e9:.4f} GHz")

            sub_df = df[df['Frequency_Hz'] == nearest_freq].sort_values(by='Angle_deg')
            plot_angles = sub_df['Angle_deg'].values
            plot_mags = sub_df['Magnitude_dB'].values

            # Matplotlib Polar plot
            plt.figure(figsize=(8, 8))
            ax = plt.subplot(111, projection='polar')
            
            # Convert degrees to radians
            angles_rad = np.radians(plot_angles)
            
            # Plot
            ax.plot(angles_rad, plot_mags, color='crimson', linewidth=2.5, marker='o', markersize=4)
            ax.set_theta_zero_location('N')  # Set 0 deg at top
            ax.set_theta_direction(-1)       # Clockwise
            
            # Set grid and labels
            ax.set_rlabel_position(135)      # Move radial labels out of the way
            
            plt.title(f'2D Radiation Pattern at {nearest_freq / 1e9:.4f} GHz\n({base_name})', pad=20)
            plt.tight_layout()
            
            polar_img = f'{base_name}_polar_{nearest_freq/1e9:.3f}ghz_plot.png'
            plt.savefig(polar_img, dpi=300)
            print(f"[SUCCESS] Polar plot saved as: {polar_img}")
            plt.show()

if __name__ == '__main__':
    main()
