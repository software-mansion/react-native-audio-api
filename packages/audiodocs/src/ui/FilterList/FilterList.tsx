import FormControl from '@mui/material/FormControl';
import InputLabel from '@mui/material/InputLabel';
import MenuItem from '@mui/material/MenuItem';
import Select, { SelectChangeEvent } from '@mui/material/Select';
import React, { useCallback, useId } from 'react';
import styles from './styles.module.css';

export interface FilterListOption<T extends string = string> {
  value: T;
  label: string;
}

interface FilterListProps<T extends string = string> {
  options: Array<FilterListOption<T>>;
  value: T;
  onChange: (value: T) => void;
  ariaLabel?: string;
  className?: string;
}

const FilterList = <T extends string>({
  options,
  value,
  onChange,
  ariaLabel = 'Filter type',
  className,
}: FilterListProps<T>) => {
  const labelId = useId();

  const handleSelectChange = useCallback((event: SelectChangeEvent<string>) => {
    onChange(event.target.value as T);
  }, [onChange]);

  return (
    <div className={className ? `${styles.container} ${className}` : styles.container}>
      <div className={styles.buttonList} role="radiogroup" aria-label={ariaLabel}>
        {options.map((option) => (
          <button
            key={option.value}
            type="button"
            onClick={() => onChange(option.value)}
            className={option.value === value ? `${styles.button} ${styles.buttonActive}` : styles.button}
            aria-pressed={option.value === value}
          >
            {option.label}
          </button>
        ))}
      </div>

      <div className={styles.mobileSelectContainer}>
        <FormControl fullWidth size="small" className={styles.formControl}>
          <InputLabel id={labelId} className={styles.inputLabel}>
            Filter type
          </InputLabel>
          <Select
            labelId={labelId}
            className={styles.select}
            value={value}
            label="Filter type"
            onChange={handleSelectChange}
            inputProps={{ 'aria-label': ariaLabel }}
            MenuProps={{
              PaperProps: { className: styles.menuPaper },
            }}
          >
            {options.map((option) => (
              <MenuItem key={option.value} value={option.value} className={styles.menuItem}>
                {option.label}
              </MenuItem>
            ))}
          </Select>
        </FormControl>
      </div>
    </div>
  );
};

export default FilterList;
