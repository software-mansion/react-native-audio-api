import React, { Children, PropsWithChildren } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors, layout } from '../styles';
import withSeparators from '../utils/withSeparators';

type ListSectionProps = PropsWithChildren<{
  title?: string;
  /** Where row separators start; defaults to past the 30 pt row badge. */
  separatorInset?: number;
}>;

/**
 * An inset grouped section (iOS Settings style). The header is inset by the
 * group's corner radius so it lines up with where the rounded corner ends.
 * Separators start past the 30 pt row badge, aligned with the row titles.
 */
const ListSection: React.FC<ListSectionProps> = ({
  title,
  separatorInset = badgedSeparatorInset,
  children,
}) => {
  const rows = Children.toArray(children);

  return (
    <View style={styles.section}>
      {!!title && (
        <Text style={styles.title} accessibilityRole="header">
          {title}
        </Text>
      )}
      <View style={styles.group}>
        {withSeparators(
          rows,
          (index) => (
            <View
              key={`separator-${index}`}
              style={[styles.separator, { marginLeft: separatorInset }]}
            />
          ),
          (row) => row
        )}
      </View>
    </View>
  );
};

export default ListSection;

const rowInset = 12;
const badgeSize = 30;
const badgeGap = 12;
const badgedSeparatorInset = rowInset + badgeSize + badgeGap;

const styles = StyleSheet.create({
  section: {
    marginBottom: 24,
  },
  title: {
    fontSize: 13,
    fontWeight: '600',
    letterSpacing: 0.4,
    textTransform: 'uppercase',
    color: colors.textMuted,
    paddingHorizontal: layout.cardRadius,
    marginBottom: 8,
  },
  group: {
    borderRadius: layout.cardRadius,
    overflow: 'hidden',
    backgroundColor: colors.surface,
  },
  separator: {
    height: StyleSheet.hairlineWidth,
    backgroundColor: colors.listSeparator,
  },
});
