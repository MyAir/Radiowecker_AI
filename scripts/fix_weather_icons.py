import os
import re

def fix_icon_files(directory):
    # Pattern to match icon files
    pattern = re.compile(r'icon_(\d+[dn])\.c')
    
    # Process each file in the directory
    for filename in os.listdir(directory):
        match = pattern.match(filename)
        if match:
            icon_code = match.group(1)  # e.g., '01d', '02n', etc.
            filepath = os.path.join(directory, filename)
            
            # Read the file content
            with open(filepath, 'r') as f:
                content = f.read()
            
            # Replace variable names to be valid C identifiers
            new_content = content
            
            # Replace array declarations (e.g., 'uint8_t 01d_map[]' -> 'uint8_t icon_01d_map[]')
            new_content = re.sub(
                r'(const\s+LV_ATTRIBUTE_MEM_ALIGN\s+LV_ATTRIBUTE_LARGE_CONST\s+LV_ATTRIBUTE_IMG_\w+\s+uint8_t\s+)(\d+[dn]_map)\[\]',
                r'\1icon_\2[]',
                new_content
            )
            
            # Replace image descriptor declarations (e.g., 'const lv_img_dsc_t 01d' -> 'const lv_img_dsc_t icon_01d')
            new_content = re.sub(
                r'(const\s+lv_img_dsc_t\s+)(\d+[dn])\s*=',
                r'\1icon_\2 =',
                new_content
            )
            
            # Replace references to the array (e.g., '.data = 01d_map' -> '.data = icon_01d_map')
            new_content = re.sub(
                r'(\.data\s*=\s*)(\d+[dn]_map)',
                r'\1icon_\2',
                new_content
            )
            
            # If the content was modified, write it back to the file
            if new_content != content:
                with open(filepath, 'w') as f:
                    f.write(new_content)
                print(f"Fixed: {filename}")
            else:
                print(f"No changes needed for: {filename}")

if __name__ == "__main__":
    # Path to the weather_icons directory
    weather_icons_dir = os.path.join("src", "weather_icons")
    
    # Make sure the directory exists
    if os.path.exists(weather_icons_dir) and os.path.isdir(weather_icons_dir):
        fix_icon_files(weather_icons_dir)
    else:
        print(f"Error: Directory not found: {weather_icons_dir}")
